#!/usr/bin/env python3
"""
Offline validation for portable map layout ROM packs: metatile directory, optional
dimensions directory, optional tileset-indices directory.

Dimensions rows: **width** and **height** must be non-zero when the row is non-zero;
**borderWidth** / **borderHeight** may be zero (``border_words == 0`` in the metatile row).

See docs/portable_rom_map_layout_metatiles.md.
Map/border **grid** payloads referenced by packs are still **`u16` block words** (`global.fieldmap.h`);
semantics: docs/architecture/project_c_metatile_id_map_format.md; widening: project_c_map_block_word_and_tooling_boundary.md.
Optional **`--validate-block-words`** checks each blob as vanilla **`u16`** words (even byte length;
optional metatile id ceiling — keep in sync with **`include/constants/map_grid_block_word.h`**).

At least one of **--metatiles-off**, **--tileset-indices-off**, or **--dimensions-off** is required
(dimensions-only validates the geometry table alone).

Optional **--layouts-json** (``data/layouts/layouts.json``) sets **--slot-count** from ``len(layouts)`` and,
unless **--null-slots** is passed, treats slots **without** an ``id`` field as NULL ``gMapLayouts[]`` rows
(matching ``generate_portable_map_data.py`` / ``build_map_layout_metatiles_rom_pack.py``).
With **--tileset-indices-off**, **--compiled-tileset-count** may be omitted and is inferred from the same
unique tileset symbol ordering as ``generate_portable_map_data.py`` (layouts slice); if both are passed,
they must agree.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from map_visual_policy_constants import (
    DEFAULT_WIRE_V1_METATILE_ID_LIMIT_EXCLUSIVE,
    MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK,
)

METATILE_ROW = 16
DIM_ROW = 8
TILESET_IDX_ROW = 4
MAX_SLOTS = 512
COMPILED_TILESET_SENTINEL = 0xFFFF

# Must match ``generate_portable_map_data.py`` / ``build_map_layout_tileset_indices_rom_pack.py`` (layouts-only).
def compiled_tileset_table_len_from_layouts(layouts: list) -> int:
    tilesets: set[str] = set()
    for row in layouts:
        if not isinstance(row, dict) or "id" not in row:
            continue
        tilesets.add(row["primary_tileset"])
        tilesets.add(row["secondary_tileset"])
    return len(sorted(t for t in tilesets if t != "NULL"))


# ROM map/border blobs are PRET **wire V1** on disk.
BLOCK_WORD_METATILE_MASK = MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK
DEFAULT_BLOCK_WORD_METATILE_LIMIT_EXCL = DEFAULT_WIRE_V1_METATILE_ID_LIMIT_EXCLUSIVE


def validate_map_block_word_blob(data: bytes, *, metatile_limit_excl: int) -> str | None:
    if len(data) % 2 != 0:
        return f"odd byte length {len(data)}"
    if metatile_limit_excl < 1 or metatile_limit_excl > BLOCK_WORD_METATILE_MASK + 1:
        return f"invalid metatile limit {metatile_limit_excl}"
    for i in range(0, len(data), 2):
        w = int.from_bytes(data[i : i + 2], "little")
        mid = w & BLOCK_WORD_METATILE_MASK
        if mid >= metatile_limit_excl:
            return f"word[{i // 2}]=0x{w:04X} metatile_id={mid} >= {metatile_limit_excl}"
    return None


def read_le_u32(b: bytes, off: int) -> int:
    return int.from_bytes(b[off : off + 4], "little")


def read_le_u16(b: bytes, off: int) -> int:
    return int.from_bytes(b[off : off + 2], "little")


def parse_hex(s: str) -> int:
    s = s.strip()
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def validate_dimensions_table(rom: bytes, dim_off: int, slot_count: int, null_set: set[int]) -> str | None:
    dim_bytes = slot_count * DIM_ROW
    if dim_off < 0 or dim_bytes > len(rom) or dim_off > len(rom) - dim_bytes:
        return "dimensions directory out of ROM range"
    for i in range(slot_count):
        d = dim_off + i * DIM_ROW
        w = read_le_u16(rom, d + 0)
        h = read_le_u16(rom, d + 2)
        bbw = read_le_u16(rom, d + 4)
        bbh = read_le_u16(rom, d + 6)
        s = w | h | bbw | bbh
        if i in null_set:
            if s != 0:
                return f"slot {i}: null slot must have zero dimensions row"
            continue
        if s == 0:
            continue
        if w == 0 or h == 0:
            return f"slot {i}: dimensions row must have non-zero width and height"
        if bbw > 255 or bbh > 255:
            return f"slot {i}: borderWidth/borderHeight must fit u8 (<=255)"
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--metatiles-off", default=None, help="hex offset to metatile directory (optional if tileset-only)")
    ap.add_argument(
        "--slot-count",
        type=int,
        default=None,
        help="gMapLayoutsSlotCount (optional if --layouts-json supplies the table)",
    )
    ap.add_argument(
        "--layouts-json",
        type=Path,
        default=None,
        help="data/layouts/layouts.json: set slot-count=len(layouts); if --null-slots omitted, infer NULL slots (no id)",
    )
    ap.add_argument("--dimensions-off", default=None, help="hex offset to dimensions directory (optional)")
    ap.add_argument(
        "--null-slots",
        default="",
        help="comma-separated slot indices (0-based) that must have zero metatile rows",
    )
    ap.add_argument("--tileset-indices-off", default=None, help="hex offset to tileset index directory (optional)")
    ap.add_argument(
        "--compiled-tileset-count",
        type=int,
        default=None,
        help="gFireredPortableCompiledTilesetTableCount (with --tileset-indices-off: inferred from --layouts-json if omitted)",
    )
    ap.add_argument(
        "--validate-block-words",
        action="store_true",
        help="check map/border blobs as u16 block words (requires --metatiles-off)",
    )
    ap.add_argument(
        "--block-word-metatile-limit",
        type=int,
        default=DEFAULT_BLOCK_WORD_METATILE_LIMIT_EXCL,
        metavar="EXCLUSIVE_MAX",
        help="with --validate-block-words: reject if (word & 0x3FF) >= limit (default 1024)",
    )
    args = ap.parse_args()

    inferred_null: set[int] | None = None
    layouts_for_infer: list | None = None
    if args.layouts_json is not None:
        lj = json.loads(args.layouts_json.read_text(encoding="utf-8"))
        layouts = lj.get("layouts")
        if not isinstance(layouts, list):
            print("layouts-json: missing layouts[]", file=sys.stderr)
            return 2
        layouts_for_infer = layouts
        inferred_count = len(layouts)
        inferred_null = {i for i, row in enumerate(layouts) if not isinstance(row, dict) or "id" not in row}
        if args.slot_count is None:
            args.slot_count = inferred_count
        elif args.slot_count != inferred_count:
            print(
                f"slot-count ({args.slot_count}) != len(layouts) from {args.layouts_json} ({inferred_count})",
                file=sys.stderr,
            )
            return 2
    elif args.slot_count is None:
        print("need --slot-count or --layouts-json", file=sys.stderr)
        return 2

    if args.slot_count < 1 or args.slot_count > MAX_SLOTS:
        print(f"slot-count must be 1..{MAX_SLOTS}", file=sys.stderr)
        return 2

    if args.metatiles_off is None and args.tileset_indices_off is None and args.dimensions_off is None:
        print(
            "need at least one of --metatiles-off, --tileset-indices-off, or --dimensions-off",
            file=sys.stderr,
        )
        return 2

    if args.validate_block_words and args.metatiles_off is None:
        print("--validate-block-words requires --metatiles-off", file=sys.stderr)
        return 2

    if args.tileset_indices_off is not None:
        if args.compiled_tileset_count is None:
            if layouts_for_infer is None:
                print(
                    "--tileset-indices-off requires --compiled-tileset-count or --layouts-json",
                    file=sys.stderr,
                )
                return 2
            args.compiled_tileset_count = compiled_tileset_table_len_from_layouts(layouts_for_infer)
        elif layouts_for_infer is not None:
            inferred_tc = compiled_tileset_table_len_from_layouts(layouts_for_infer)
            if inferred_tc != args.compiled_tileset_count:
                print(
                    f"--compiled-tileset-count {args.compiled_tileset_count} != "
                    f"layouts-json inferred table len {inferred_tc}",
                    file=sys.stderr,
                )
                return 2

    rom = args.rom.read_bytes()
    null_set: set[int] = set()
    if args.null_slots.strip():
        for p in args.null_slots.split(","):
            p = p.strip()
            if not p:
                continue
            null_set.add(int(p, 0))
    elif inferred_null is not None:
        null_set = inferred_null

    dim_off: int | None = parse_hex(args.dimensions_off) if args.dimensions_off else None
    if dim_off is not None:
        err = validate_dimensions_table(rom, dim_off, args.slot_count, null_set)
        if err is not None:
            print(err, file=sys.stderr)
            return 1

    warned_compiled_geom = False

    if args.metatiles_off is not None:
        meta_off = parse_hex(args.metatiles_off)

        meta_bytes = args.slot_count * METATILE_ROW
        if meta_off < 0 or meta_bytes > len(rom) or meta_off > len(rom) - meta_bytes:
            print("metatile directory out of ROM range", file=sys.stderr)
            return 1

        for i in range(args.slot_count):
            row = meta_off + i * METATILE_ROW
            bo = read_le_u32(rom, row + 0)
            mo = read_le_u32(rom, row + 4)
            bw = read_le_u32(rom, row + 8)
            mw = read_le_u32(rom, row + 12)
            allzero = bo | mo | bw | mw

            if i in null_set:
                if allzero != 0:
                    print(f"slot {i}: expected zero metatile row (null layout)", file=sys.stderr)
                    return 1
                continue

            if allzero == 0:
                continue

            if (bo | mo) & 1:
                print(f"slot {i}: metatile offsets must be 2-byte aligned", file=sys.stderr)
                return 1

            border_bytes = bw * 2
            map_bytes = mw * 2
            if bo > len(rom) or border_bytes > len(rom) - bo:
                print(f"slot {i}: border blob out of range", file=sys.stderr)
                return 1
            if mo > len(rom) or map_bytes > len(rom) - mo:
                print(f"slot {i}: map blob out of range", file=sys.stderr)
                return 1

            if args.validate_block_words:
                bdata = rom[bo : bo + border_bytes]
                mdata = rom[mo : mo + map_bytes]
                err = validate_map_block_word_blob(bdata, metatile_limit_excl=args.block_word_metatile_limit)
                if err:
                    print(f"slot {i} border blob: {err}", file=sys.stderr)
                    return 1
                err = validate_map_block_word_blob(mdata, metatile_limit_excl=args.block_word_metatile_limit)
                if err:
                    print(f"slot {i} map blob: {err}", file=sys.stderr)
                    return 1

            if dim_off is None:
                continue

            d = dim_off + i * DIM_ROW
            w = read_le_u16(rom, d + 0)
            h = read_le_u16(rom, d + 2)
            bbw = read_le_u16(rom, d + 4)
            bbh = read_le_u16(rom, d + 6)
            dsum = w | h | bbw | bbh
            if dsum == 0:
                warned_compiled_geom = True
                continue

            exp_bw = bbw * bbh
            exp_mw = w * h
            if bw != exp_bw or mw != exp_mw:
                print(
                    f"slot {i}: word counts mismatch vs ROM dimensions want border_words={exp_bw} "
                    f"map_words={exp_mw} got {bw} {mw}",
                    file=sys.stderr,
                )
                return 1

        if warned_compiled_geom:
            print(
                "note: one or more slots use non-zero metatile rows with all-zero dimensions rows "
                "(compiled geometry); word counts were not cross-checked vs MapLayout.",
                file=sys.stderr,
            )

    if args.tileset_indices_off is not None:
        ts_off = parse_hex(args.tileset_indices_off)
        tc = args.compiled_tileset_count
        if tc < 1:
            print("compiled-tileset-count must be >= 1", file=sys.stderr)
            return 2
        ts_bytes = args.slot_count * TILESET_IDX_ROW
        if ts_off < 0 or ts_bytes > len(rom) or ts_off > len(rom) - ts_bytes:
            print("tileset indices directory out of ROM range", file=sys.stderr)
            return 1

        for i in range(args.slot_count):
            row = ts_off + i * TILESET_IDX_ROW
            pi = read_le_u16(rom, row + 0)
            si = read_le_u16(rom, row + 2)
            in_null = i in null_set
            if in_null:
                if pi != COMPILED_TILESET_SENTINEL or si != COMPILED_TILESET_SENTINEL:
                    print(
                        f"slot {i}: null layout tileset row must be 0xFFFF,0xFFFF",
                        file=sys.stderr,
                    )
                    return 1
                continue
            if pi != COMPILED_TILESET_SENTINEL and pi >= tc:
                print(f"slot {i}: primary tileset index {pi} >= compiled table count {tc}", file=sys.stderr)
                return 1
            if si != COMPILED_TILESET_SENTINEL and si >= tc:
                print(f"slot {i}: secondary tileset index {si} >= compiled table count {tc}", file=sys.stderr)
                return 1

    print("OK: map layout ROM packs validated.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
