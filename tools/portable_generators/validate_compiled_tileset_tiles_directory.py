#!/usr/bin/env python3
"""
Structural checks for FIRERED_ROM_COMPILED_TILESET_TILES_DIRECTORY_OFF.

One u32 offset per gFireredPortableCompiledTilesetTable index. Non-zero offsets must be
4-byte aligned.

Each blob may be either:
  - Raw uncompressed tile data (max --max-blob bytes, default = --pri-uncomp-bytes), or
  - GBA LZ77: first byte 0x10, bytes 1-3 little-endian decompressed size in
    {--pri-uncomp-bytes, --sec-uncomp-bytes} (vanilla 20480 / 12288).

Usage:
  python validate_compiled_tileset_tiles_directory.py <rom.gba> --off 0x... --count N
  python validate_compiled_tileset_tiles_directory.py <rom.gba> --off 0x... --count N \\
      --pri-uncomp-bytes 32768 --sec-uncomp-bytes 16384
  python validate_compiled_tileset_tiles_directory.py <rom.gba> --off 0x... --count N \\
      --enforce-role-tile-caps
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

from compiled_tileset_table_roles import build_role_tile_caps_for_enforcement
from map_visual_policy_constants import (
    DEFAULT_PRI_UNCOMPRESSED_BYTES,
    DEFAULT_SEC_UNCOMPRESSED_BYTES,
    MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES,
)

ROW_BYTES = 4
LZ_MAGIC = 0x10
PRI_UNCOMPRESSED = DEFAULT_PRI_UNCOMPRESSED_BYTES
SEC_UNCOMPRESSED = DEFAULT_SEC_UNCOMPRESSED_BYTES
DEFAULT_MAX_BLOB = PRI_UNCOMPRESSED
REPO_ROOT = Path(__file__).resolve().parents[2]


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to directory start")
    ap.add_argument("--count", type=int, required=True, metavar="N", help="gFireredPortableCompiledTilesetTableCount")
    ap.add_argument(
        "--enforce-role-tile-caps",
        action="store_true",
        help="LZ decompressed size must match primary vs secondary cap per table index; raw max uses min(--max-blob, role cap)",
    )
    ap.add_argument(
        "--compiled-tileset-table-c",
        type=Path,
        default=None,
        help="map_data_portable.c (default: pokefirered_core/generated/.../map_data_portable.c)",
    )
    ap.add_argument(
        "--tileset-headers",
        type=Path,
        default=None,
        help="tileset headers.h (default: src/data/tilesets/headers.h)",
    )
    ap.add_argument(
        "--pri-tile-cap",
        type=int,
        default=None,
        metavar="BYTES",
        help=f"override primary cap when enforcing roles (default {DEFAULT_PRI_UNCOMPRESSED_BYTES})",
    )
    ap.add_argument(
        "--sec-tile-cap",
        type=int,
        default=None,
        metavar="BYTES",
        help=f"override secondary cap when enforcing roles (default {DEFAULT_SEC_UNCOMPRESSED_BYTES})",
    )
    ap.add_argument(
        "--pri-uncomp-bytes",
        type=int,
        default=DEFAULT_PRI_UNCOMPRESSED,
        metavar="BYTES",
        help="expected primary tile sheet decompressed size (LZ header or raw max default anchor)",
    )
    ap.add_argument(
        "--sec-uncomp-bytes",
        type=int,
        default=DEFAULT_SEC_UNCOMPRESSED,
        metavar="BYTES",
        help="expected secondary tile sheet decompressed size for LZ header check",
    )
    ap.add_argument(
        "--max-blob",
        type=int,
        default=None,
        metavar="BYTES",
        help="max bytes for raw uncompressed blobs (default: --pri-uncomp-bytes)",
    )
    args = ap.parse_args()

    if args.count < 0:
        print("count must be non-negative", file=sys.stderr)
        return 2
    max_blob = args.max_blob if args.max_blob is not None else args.pri_uncomp_bytes
    if max_blob < 1:
        print("max-blob must be positive", file=sys.stderr)
        return 2
    if max_blob > MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES:
        print(
            f"max-blob {max_blob} exceeds MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES ({MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES})",
            file=sys.stderr,
        )
        return 2
    if args.pri_uncomp_bytes < 1 or args.sec_uncomp_bytes < 1:
        print("pri/sec uncomp bytes must be positive", file=sys.stderr)
        return 2
    if args.pri_uncomp_bytes > MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES or args.sec_uncomp_bytes > MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES:
        print(
            f"pri/sec uncomp bytes must be <= MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES ({MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES})",
            file=sys.stderr,
        )
        return 2

    role_caps: list[int] | None = None
    role_symbols: list[str] | None = None
    if args.enforce_role_tile_caps:
        try:
            role_caps, role_symbols = build_role_tile_caps_for_enforcement(
                count=args.count,
                repo_root=REPO_ROOT,
                compiled_tileset_table_c=args.compiled_tileset_table_c,
                tileset_headers=args.tileset_headers,
                pri_cap=args.pri_tile_cap,
                sec_cap=args.sec_tile_cap,
            )
        except FileNotFoundError as e:
            print(f"--enforce-role-tile-caps: missing {e}", file=sys.stderr)
            return 2
        except ValueError as e:
            print(f"role cap setup: {e}", file=sys.stderr)
            return 2

    need = args.count * ROW_BYTES
    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or need > len(rom) or off > len(rom) - need:
        print("compiled tileset tiles directory does not fit in ROM at given offset", file=sys.stderr)
        return 1

    allowed_uncomp = {args.pri_uncomp_bytes, args.sec_uncomp_bytes}

    for i in range(args.count):
        o = struct.unpack_from("<I", rom, off + i * ROW_BYTES)[0]
        if o == 0:
            continue
        if o & 3:
            print(f"index {i}: offset not 4-byte aligned", file=sys.stderr)
            return 1
        if o >= len(rom):
            print(f"index {i}: offset out of ROM", file=sys.stderr)
            return 1

        per_max_blob = max_blob
        if role_caps is not None:
            per_max_blob = min(max_blob, role_caps[i])

        if rom[o] == LZ_MAGIC and o + 4 <= len(rom):
            uncomp = rom[o + 1] | (rom[o + 2] << 8) | (rom[o + 3] << 16)
            if role_caps is not None:
                if uncomp != role_caps[i]:
                    sym = role_symbols[i] if role_symbols else "?"
                    print(
                        f"index {i} ({sym}): LZ77 decompressed size {uncomp} != role cap {role_caps[i]}",
                        file=sys.stderr,
                    )
                    return 1
                continue
            if uncomp in allowed_uncomp:
                continue
            print(
                f"index {i}: LZ77 header has unexpected decompressed size {uncomp} "
                f"(expected one of {sorted(allowed_uncomp)})",
                file=sys.stderr,
            )
            return 1

        if o + per_max_blob > len(rom):
            print(
                f"index {i}: raw tile blob out of ROM bounds (max {per_max_blob} bytes)",
                file=sys.stderr,
            )
            return 1

    extra = " + role tile caps" if role_caps is not None else ""
    print(f"OK: compiled tileset tiles directory valid ({args.count} u32 rows{extra}).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
