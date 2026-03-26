#!/usr/bin/env python3
"""
Structural offline checks for FIRERED_ROM_MAP_LAYOUT_METATILE_ATTRIBUTES_DIRECTORY_OFF.

Map-grid id span is documented in Project C (`MapGridMetatileIdIsInEncodedSpace`); this script validates ROM **blob sizes** only.

Mirrors layout-slot validation in try_apply_metatile_attributes_from_rom (bounds + alignment).
Default expected blob sizes match MAP_BG / fieldmap.h: primary 640 u32, secondary 384 u32.
Override with --pri-attr-bytes / --sec-attr-bytes for extended profile packs.
If overrides exceed those vanilla caps, pass --allow-exceed-map-bg-metatile-attr (Direction D).

Usage:
  python validate_map_layout_metatile_attributes_directory.py <rom.gba> --off 0x... --slots N
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

from map_visual_policy_constants import (
    DEFAULT_PRIMARY_METATILE_ATTR_BYTES,
    DEFAULT_SECONDARY_METATILE_ATTR_BYTES,
)

ROW_BYTES = 8
DEFAULT_PRI_ATTR_BYTES = DEFAULT_PRIMARY_METATILE_ATTR_BYTES
DEFAULT_SEC_ATTR_BYTES = DEFAULT_SECONDARY_METATILE_ATTR_BYTES


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to directory start")
    ap.add_argument("--slots", type=int, required=True, metavar="N", help="gMapLayoutsSlotCount")
    ap.add_argument(
        "--pri-attr-bytes",
        type=int,
        default=DEFAULT_PRI_ATTR_BYTES,
        metavar="N",
        help=f"expected primary attributes blob (default {DEFAULT_PRI_ATTR_BYTES})",
    )
    ap.add_argument(
        "--sec-attr-bytes",
        type=int,
        default=DEFAULT_SEC_ATTR_BYTES,
        metavar="N",
        help=f"expected secondary attributes blob (default {DEFAULT_SEC_ATTR_BYTES})",
    )
    ap.add_argument(
        "--allow-exceed-map-bg-metatile-attr",
        action="store_true",
        help="allow pri/sec attribute blob sizes above vanilla MAP_BG caps",
    )
    args = ap.parse_args()

    if args.slots < 0:
        print("slots must be non-negative", file=sys.stderr)
        return 2

    need = args.slots * ROW_BYTES
    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or need > len(rom) or off > len(rom) - need:
        print("metatile attributes directory does not fit in ROM at given offset", file=sys.stderr)
        return 1

    pri_bytes = args.pri_attr_bytes
    sec_bytes = args.sec_attr_bytes

    for i in range(args.slots):
        po, so = struct.unpack_from("<2I", rom, off + i * ROW_BYTES)
        if po != 0:
            if po & 3:
                print(f"slot {i}: primary_attributes_off not 4-byte aligned", file=sys.stderr)
                return 1
            if po > len(rom) or pri_bytes > len(rom) - po:
                print(f"slot {i}: primary attributes blob out of ROM bounds", file=sys.stderr)
                return 1
        if so != 0:
            if so & 3:
                print(f"slot {i}: secondary_attributes_off not 4-byte aligned", file=sys.stderr)
                return 1
            if so > len(rom) or sec_bytes > len(rom) - so:
                print(f"slot {i}: secondary attributes blob out of ROM bounds", file=sys.stderr)
                return 1

    print(
        f"OK: metatile attributes directory valid ({args.slots} rows, "
        f"pri {args.pri_attr_bytes} / sec {args.sec_attr_bytes} byte blobs)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
