#!/usr/bin/env python3
"""
Structural checks for FIRERED_ROM_COMPILED_TILESET_PALETTES_DIRECTORY_OFF.

One u32 offset per gFireredPortableCompiledTilesetTable index. Non-zero offsets must be
2-byte aligned and fit the expected blob size (default vanilla 13 * 32 = 416). When using
FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF, non-zero palette_bundle_bytes per index
define the expected size — pass the matching per-pack --blob-bytes (uniform rows) or run
per-index checks offline. Runtime row bounds use FireredPortableCompiledTilesetPaletteRowPtr.

Usage:
  python validate_compiled_tileset_palettes_directory.py <rom.gba> --off 0x... --count N
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

ROW_BYTES = 4
# Mirrors fieldmap.h: NUM_PALS_TOTAL * PLTT_SIZE_4BPP
BLOB_BYTES = 13 * 32


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to directory start")
    ap.add_argument("--count", type=int, required=True, metavar="N", help="gFireredPortableCompiledTilesetTableCount")
    ap.add_argument(
        "--blob-bytes",
        type=int,
        default=BLOB_BYTES,
        metavar="BYTES",
        help=f"bytes per non-zero palette blob (default {BLOB_BYTES})",
    )
    args = ap.parse_args()

    if args.count < 0:
        print("count must be non-negative", file=sys.stderr)
        return 2
    if args.blob_bytes < 1:
        print("blob-bytes must be positive", file=sys.stderr)
        return 2

    need = args.count * ROW_BYTES
    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or need > len(rom) or off > len(rom) - need:
        print("compiled tileset palettes directory does not fit in ROM at given offset", file=sys.stderr)
        return 1

    for i in range(args.count):
        o = struct.unpack_from("<I", rom, off + i * ROW_BYTES)[0]
        if o == 0:
            continue
        if o & 1:
            print(f"index {i}: offset not 2-byte aligned", file=sys.stderr)
            return 1
        if o > len(rom) or args.blob_bytes > len(rom) - o:
            print(f"index {i}: palette blob out of ROM bounds ({args.blob_bytes} bytes)", file=sys.stderr)
            return 1

    print(f"OK: compiled tileset palettes directory valid ({args.count} u32 rows).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
