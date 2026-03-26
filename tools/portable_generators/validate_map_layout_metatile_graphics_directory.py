#!/usr/bin/env python3
"""
Structural checks for FIRERED_ROM_MAP_LAYOUT_METATILE_GRAPHICS_DIRECTORY_OFF.

Runtime map-grid metatile ids are capped by **`MapGridMetatileIdIsInEncodedSpace`** / 10-bit **`MAPGRID_METATILE_ID_MASK`**
(`docs/architecture/project_c_metatile_id_map_format.md`); these validators only size ROM **attribute/gfx blobs**, not grid words.

Mirrors layout-slot validation: 8 bytes per slot, u16-aligned offsets.
Default expected blob sizes match vanilla MAP_BG / fieldmap.h (Project C **`MAP_GRID_METATILE_*`**):
  primary   MAP_GRID_METATILE_PRIMARY_COUNT metatiles × 8 halfwords
  secondary NUM_METATILES_IN_SECONDARY (384 vanilla) metatiles × 8 halfwords

When using FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF metatile_gfx_bytes overrides,
pass matching --pri-gfx-bytes / --sec-gfx-bytes. If sizes exceed vanilla MAP_BG caps,
pass --allow-exceed-map-bg-metatile-gfx (Direction D: explicit opt-in to extended tables).

Usage:
  python validate_map_layout_metatile_graphics_directory.py <rom.gba> --off 0x... --slots N
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

from map_visual_policy_constants import (
    DEFAULT_PRIMARY_METATILE_GFX_BYTES,
    DEFAULT_SECONDARY_METATILE_GFX_BYTES,
    MAP_BG_MAX_PRI_GFX_BYTES,
    MAP_BG_MAX_SEC_GFX_BYTES,
)

ROW_BYTES = 8
DEFAULT_PRIMARY_BYTES = DEFAULT_PRIMARY_METATILE_GFX_BYTES
DEFAULT_SECONDARY_BYTES = DEFAULT_SECONDARY_METATILE_GFX_BYTES


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to directory start")
    ap.add_argument("--slots", type=int, required=True, metavar="N")
    ap.add_argument(
        "--pri-gfx-bytes",
        type=int,
        default=DEFAULT_PRIMARY_BYTES,
        metavar="N",
        help=f"expected primary metatile graphics blob size (default {DEFAULT_PRIMARY_BYTES})",
    )
    ap.add_argument(
        "--sec-gfx-bytes",
        type=int,
        default=DEFAULT_SECONDARY_BYTES,
        metavar="N",
        help=f"expected secondary metatile graphics blob size (default {DEFAULT_SECONDARY_BYTES})",
    )
    ap.add_argument(
        "--allow-exceed-map-bg-metatile-gfx",
        action="store_true",
        help="allow pri/sec gfx byte sizes above vanilla MAP_BG metatile graphics caps",
    )
    args = ap.parse_args()

    if args.slots < 0:
        print("slots must be non-negative", file=sys.stderr)
        return 2

    if not args.allow_exceed_map_bg_metatile_gfx:
        if args.pri_gfx_bytes > MAP_BG_MAX_PRI_GFX_BYTES or args.sec_gfx_bytes > MAP_BG_MAX_SEC_GFX_BYTES:
            print(
                "pri-gfx-bytes / sec-gfx-bytes exceed vanilla MAP_BG metatile graphics caps "
                f"({MAP_BG_MAX_PRI_GFX_BYTES} / {MAP_BG_MAX_SEC_GFX_BYTES}). "
                "Use --allow-exceed-map-bg-metatile-gfx for extended profile packs.",
                file=sys.stderr,
            )
            return 2

    need = args.slots * ROW_BYTES
    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or need > len(rom) or off > len(rom) - need:
        print("metatile graphics directory does not fit in ROM at given offset", file=sys.stderr)
        return 1

    for i in range(args.slots):
        po, so = struct.unpack_from("<2I", rom, off + i * ROW_BYTES)
        if po != 0:
            if po & 1:
                print(f"slot {i}: primary_metatiles_off not u16-aligned", file=sys.stderr)
                return 1
            if po > len(rom) or args.pri_gfx_bytes > len(rom) - po:
                print(f"slot {i}: primary metatile graphics blob out of ROM bounds", file=sys.stderr)
                return 1
        if so != 0:
            if so & 1:
                print(f"slot {i}: secondary_metatiles_off not u16-aligned", file=sys.stderr)
                return 1
            if so > len(rom) or args.sec_gfx_bytes > len(rom) - so:
                print(f"slot {i}: secondary metatile graphics blob out of ROM bounds", file=sys.stderr)
                return 1

    print(
        f"OK: metatile graphics directory valid ({args.slots} rows, "
        f"pri {args.pri_gfx_bytes} / sec {args.sec_gfx_bytes} bytes)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
