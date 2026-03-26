#!/usr/bin/env python3
"""
Emit a contiguous image for FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF:

  - Metatile directory at **byte offset 0** in the emitted file: ``gMapLayoutsSlotCount × 16`` bytes.
  - Border / map **PRET wire V1** ``u16`` LE blobs appended; each row stores **absolute file offsets**
    into the **host ROM image** (same as ``firered_portable_rom_map_layout_metatiles.c``: ``rom + off``),
    **not** GBA ``0x08……`` pointers.

Source order matches ``tools/portable_generators/generate_portable_map_data.py``: ``data/layouts/layouts.json``
key ``layouts``. Entries without ``id`` are **NULL** ``gMapLayouts[]`` slots — zero rows, no blobs.

When ``border_width * border_height == 0``, **``border_words``** in the directory is **0** and no border
bytes are appended (``border_off == map_off``), even if ``border.bin`` on disk is a non-empty placeholder.

When this file is embedded at byte offset **P** inside a larger mapped ROM, pass ``--pack-base-offset P`` so
stored ``border_off`` / ``map_off`` values include **P** (directory still assumed at **P**; the emitted
file is normally placed verbatim starting at **P**).

Usage:
  python build_map_layout_metatiles_rom_pack.py <pokefirered_root> <output.bin>
  python build_map_layout_metatiles_rom_pack.py <root> <out.bin> --pack-base-offset 0x1234000

Validate (standalone pack, directory at file offset 0):
  python tools/portable_generators/validate_map_layout_rom_packs.py <out.bin> \\
    --metatiles-off 0x0 --layouts-json data/layouts/layouts.json \\
    --validate-block-words
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

METATILE_ROW = 16
MAX_SLOTS = 512


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("root", type=Path, help="PokeFireRed repo root")
    ap.add_argument("output", type=Path, help="Output .bin path")
    ap.add_argument(
        "--pack-base-offset",
        type=lambda s: int(s, 0),
        default=0,
        help="add to every stored border_off/map_off (host ROM placement; default 0)",
    )
    args = ap.parse_args()
    root = args.root.resolve()
    out_path = args.output.resolve()
    base = args.pack_base_offset
    if base < 0:
        print("pack-base-offset must be >= 0", file=sys.stderr)
        return 2

    lj_path = root / "data" / "layouts" / "layouts.json"
    if not lj_path.is_file():
        print(f"missing {lj_path}", file=sys.stderr)
        return 1

    layouts = read_json(lj_path)["layouts"]
    slot_count = len(layouts)
    if slot_count < 1 or slot_count > MAX_SLOTS:
        print(f"layouts.json length must be 1..{MAX_SLOTS}", file=sys.stderr)
        return 1

    buf = bytearray(slot_count * METATILE_ROW)
    cursor = len(buf)

    for i, row in enumerate(layouts):
        if "id" not in row:
            continue
        try:
            w = int(row["width"])
            h = int(row["height"])
            bbw = int(row["border_width"])
            bbh = int(row["border_height"])
            bpath = row["border_filepath"]
            mpath = row["blockdata_filepath"]
        except (KeyError, TypeError, ValueError) as e:
            print(f"slot {i}: bad layout JSON ({e})", file=sys.stderr)
            return 1

        expect_bw = bbw * bbh
        expect_mw = w * h
        border_file = (root / bpath).resolve()
        map_file = (root / mpath).resolve()
        if not border_file.is_file():
            print(f"slot {i}: missing border file {border_file}", file=sys.stderr)
            return 1
        if not map_file.is_file():
            print(f"slot {i}: missing map file {map_file}", file=sys.stderr)
            return 1

        border_data = border_file.read_bytes()
        map_data = map_file.read_bytes()
        if len(border_data) % 2 != 0:
            print(f"slot {i}: border.bin odd length {len(border_data)}", file=sys.stderr)
            return 1
        if len(map_data) % 2 != 0:
            print(f"slot {i}: map.bin odd length {len(map_data)}", file=sys.stderr)
            return 1
        got_mw = len(map_data) // 2
        if got_mw != expect_mw:
            print(
                f"slot {i}: map.bin word count {got_mw} != json width*height={expect_mw}",
                file=sys.stderr,
            )
            return 1

        # Directory `border_words` follows MapLayout geometry (same as portable runtime when no
        # dimensions pack). Toolchains often leave a small placeholder border.bin when
        # border_width/height are 0; we emit **zero** border bytes and set bo == mo.
        if expect_bw > 0:
            got_bw = len(border_data) // 2
            if got_bw != expect_bw:
                print(
                    f"slot {i}: border.bin words {got_bw} != json border_width*border_height={expect_bw}",
                    file=sys.stderr,
                )
                return 1
            bo = base + cursor
            buf.extend(border_data)
            cursor = len(buf)
        else:
            bo = 0  # patched after mo

        mo = base + cursor
        buf.extend(map_data)
        cursor = len(buf)
        if expect_bw == 0:
            bo = mo

        struct.pack_into("<IIII", buf, i * METATILE_ROW, bo, mo, expect_bw, expect_mw)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(buf)
    print(f"Wrote {len(buf)} bytes to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
