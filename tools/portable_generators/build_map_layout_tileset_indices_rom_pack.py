#!/usr/bin/env python3
"""
Emit a dense image for FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF.

Wire: ``gMapLayoutsSlotCount`` rows × **4 bytes** little-endian **u16**:
``primary_index``, ``secondary_index`` into ``gFireredPortableCompiledTilesetTable[]``.

**NULL** ``gMapLayouts[]`` slots (JSON object without ``id``) use **0xFFFF, 0xFFFF**
(``FIRERED_USE_COMPILED_TILESET_INDEX``) — same as portable runtime
(``firered_portable_rom_map_layout_metatiles.c``).

For non-NULL slots, each field is either **0xFFFF** (layout uses compiled ``NULL`` tileset pointer
for that role, i.e. string **``\"NULL\"``** in ``layouts.json``) or the **0-based index** of the
``gTileset_*`` symbol in the compiled table.

**Table order** must match ``generate_portable_map_data.py`` / ``map_data_portable.c``:
unique ``primary_tileset`` / ``secondary_tileset`` strings from all layouts with an ``id``,
sorted lexicographically, excluding the literal **``\"NULL\"``** symbol (not placed in the table).

Usage:
  python build_map_layout_tileset_indices_rom_pack.py <pokefirered_root> <output.bin>

Validate:
  python tools/portable_generators/validate_map_layout_rom_packs.py <out.bin> \\
    --tileset-indices-off 0x0 --layouts-json data/layouts/layouts.json
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

TILESET_IDX_ROW = 4
MAX_SLOTS = 512
SENTINEL = 0xFFFF


def compiled_tileset_syms_from_layouts(layouts: list) -> list[str]:
    """Same symbol order as generate_portable_map_data.py tileset_syms (layouts slice only)."""
    tilesets: set[str] = set()
    for row in layouts:
        if not isinstance(row, dict) or "id" not in row:
            continue
        tilesets.add(row["primary_tileset"])
        tilesets.add(row["secondary_tileset"])
    return sorted(t for t in tilesets if t != "NULL")


def sym_to_index(sym: str, index_of: dict[str, int]) -> int:
    if sym == "NULL":
        return SENTINEL
    if sym not in index_of:
        raise KeyError(sym)
    return index_of[sym]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("root", type=Path, help="PokeFireRed repo root")
    ap.add_argument("output", type=Path, help="Output .bin path")
    args = ap.parse_args()
    root = args.root.resolve()
    out_path = args.output.resolve()

    lj_path = root / "data" / "layouts" / "layouts.json"
    if not lj_path.is_file():
        print(f"missing {lj_path}", file=sys.stderr)
        return 1

    layouts = json.loads(lj_path.read_text(encoding="utf-8"))["layouts"]
    slot_count = len(layouts)
    if slot_count < 1 or slot_count > MAX_SLOTS:
        print(f"layouts.json length must be 1..{MAX_SLOTS}", file=sys.stderr)
        return 1

    syms = compiled_tileset_syms_from_layouts(layouts)
    if not syms:
        print("no non-NULL tileset symbols in layouts.json (empty compiled table)", file=sys.stderr)
        return 1

    index_of = {s: i for i, s in enumerate(syms)}
    buf = bytearray(slot_count * TILESET_IDX_ROW)

    for i, row in enumerate(layouts):
        if "id" not in row:
            struct.pack_into("<HH", buf, i * TILESET_IDX_ROW, SENTINEL, SENTINEL)
            continue
        if not isinstance(row, dict):
            print(f"slot {i}: expected object", file=sys.stderr)
            return 1
        try:
            pri = row["primary_tileset"]
            sec = row["secondary_tileset"]
        except KeyError as e:
            print(f"slot {i}: missing {e}", file=sys.stderr)
            return 1
        if not isinstance(pri, str) or not isinstance(sec, str):
            print(f"slot {i}: primary_tileset/secondary_tileset must be strings", file=sys.stderr)
            return 1
        try:
            pi = sym_to_index(pri, index_of)
            si = sym_to_index(sec, index_of)
        except KeyError as e:
            print(f"slot {i}: unknown tileset symbol {e!r} (not in compiled table order)", file=sys.stderr)
            return 1
        struct.pack_into("<HH", buf, i * TILESET_IDX_ROW, pi, si)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(buf)
    print(f"Wrote {len(buf)} bytes to {out_path} (gFireredPortableCompiledTilesetTableCount={len(syms)})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
