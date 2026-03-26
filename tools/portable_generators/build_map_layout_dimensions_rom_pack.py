#!/usr/bin/env python3
"""
Emit a dense image for FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF.

Wire: ``gMapLayoutsSlotCount`` rows × **8 bytes** little-endian **u16**:
``width``, ``height``, ``border_width``, ``border_height`` (same field names as ``layouts.json``).

Slot order matches ``generate_portable_map_data.py`` / ``build_map_layout_metatiles_rom_pack.py``.
**NULL** ``gMapLayouts[]`` slots (JSON object without ``id``) get an **all-zero** row (= use compiled
geometry for that slot at runtime, if a layout pointer exists).

For non-NULL slots, rows are copied from JSON. A **non-zero** row overrides compiled ``MapLayout``
geometry in portable runtime; **all-zero** defers to compiled geometry for that slot.

Usage:
  python build_map_layout_dimensions_rom_pack.py <pokefirered_root> <output.bin>

Validate standalone table:
  python tools/portable_generators/validate_map_layout_rom_packs.py <out.bin> \\
    --dimensions-off 0x0 --layouts-json data/layouts/layouts.json

Combined with a metatile pack (dimensions concatenated **after** the metatile image):
  python tools/portable_generators/validate_map_layout_rom_packs.py combined.bin \\
    --metatiles-off 0x0 --dimensions-off <hex len(metatile_bytes)> \\
    --layouts-json data/layouts/layouts.json --validate-block-words
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

DIM_ROW = 8
MAX_SLOTS = 512


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

    buf = bytearray(slot_count * DIM_ROW)

    for i, row in enumerate(layouts):
        if "id" not in row:
            continue
        try:
            w = int(row["width"])
            h = int(row["height"])
            bbw = int(row["border_width"])
            bbh = int(row["border_height"])
        except (KeyError, TypeError, ValueError) as e:
            print(f"slot {i}: bad layout JSON ({e})", file=sys.stderr)
            return 1
        for name, v in (("width", w), ("height", h), ("border_width", bbw), ("border_height", bbh)):
            if v < 0 or v > 65535:
                print(f"slot {i}: {name}={v} out of u16 range", file=sys.stderr)
                return 1
        struct.pack_into("<HHHH", buf, i * DIM_ROW, w, h, bbw, bbh)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(buf)
    print(f"Wrote {len(buf)} bytes to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
