#!/usr/bin/env python3
"""
Structural checks for FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF.

Legacy: 8 bytes per index (tile_uncomp_bytes, palette_bundle_bytes) — metatile fields = 0.
Extended: 16 bytes per index, adds:
  u32 metatile_gfx_bytes   (0 = vanilla size for that tileset role; non-zero: extension-only rules)
  u32 metatile_attr_bytes  (0 = vanilla; non-zero: extension-only rules)

Non-zero tile_uncomp_bytes: >= 32, multiple of 32, <= MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES
(1024 * 32 = 32768; matches include/fieldmap.h combined field char span — Direction D cap).
Non-zero palette_bundle_bytes: >= 416, multiple of 32, <= 8192.
Non-zero metatile_gfx_bytes: multiple of 16, in [6144, 32768] (min secondary vanilla, max 2048 metatiles).
Non-zero metatile_attr_bytes: multiple of 4, in [1536, 8192].

Runtime tightens metatile mins per compiled Tileset::isSecondary; offline uses loose lower bounds.

Optional **`--enforce-role-tile-caps`**: non-zero **`tile_uncomp_bytes`** must also be <= vanilla
primary (**640x32**) or secondary (**384x32**) sheet size per **`Tileset::isSecondary`** in
**`src/data/tilesets/headers.h`**, keyed by **`gFireredPortableCompiledTilesetTable`** order in
**`pokefirered_core/generated/src/data/map_data_portable.c`** (override paths if needed).

Usage:
  python validate_compiled_tileset_asset_profile_directory.py <rom.gba> --off 0x... --count N
  python validate_compiled_tileset_asset_profile_directory.py <rom.gba> --off 0x... --count 63 \\
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
    METATILE_ATTR_MAX,
    METATILE_ATTR_MIN_NONZERO,
    METATILE_ATTR_UNIT,
    METATILE_GFX_MAX,
    METATILE_GFX_MIN_NONZERO,
    METATILE_GFX_UNIT,
    PALETTE_BUNDLE_MAX_BYTES,
    VANILLA_PALETTE_BUNDLE_BYTES,
)

ROW_LEGACY = 8
ROW_EXT = 16
TILE_UNCOMP_MAX = MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES
VANILLA_PALETTE_BUNDLE = VANILLA_PALETTE_BUNDLE_BYTES
PALETTE_BUNDLE_MAX = PALETTE_BUNDLE_MAX_BYTES
REPO_ROOT = Path(__file__).resolve().parents[2]


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to profile table start")
    ap.add_argument("--count", type=int, required=True, metavar="N", help="gFireredPortableCompiledTilesetTableCount")
    ap.add_argument(
        "--enforce-role-tile-caps",
        action="store_true",
        help="tile_uncomp_bytes must fit primary (640 tiles) vs secondary (384 tiles) per headers.h + table order",
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
        help=f"override primary tile sheet cap (default {DEFAULT_PRI_UNCOMPRESSED_BYTES})",
    )
    ap.add_argument(
        "--sec-tile-cap",
        type=int,
        default=None,
        metavar="BYTES",
        help=f"override secondary tile sheet cap (default {DEFAULT_SEC_UNCOMPRESSED_BYTES})",
    )
    args = ap.parse_args()

    if args.count < 0:
        print("count must be non-negative", file=sys.stderr)
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

    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or off > len(rom):
        print("bad offset", file=sys.stderr)
        return 1

    tail = len(rom) - off
    if tail < args.count * ROW_LEGACY:
        print("compiled tileset asset profile table does not fit in ROM at given offset", file=sys.stderr)
        return 1
    if tail >= args.count * ROW_EXT:
        row_stride = ROW_EXT
    else:
        row_stride = ROW_LEGACY

    for i in range(args.count):
        row_off = off + i * row_stride
        tile_u, pal_b = struct.unpack_from("<II", rom, row_off)
        gfx_b = 0
        attr_b = 0
        if row_stride >= ROW_EXT:
            gfx_b, attr_b = struct.unpack_from("<II", rom, row_off + 8)
        if gfx_b != 0:
            if gfx_b % METATILE_GFX_UNIT != 0 or not (METATILE_GFX_MIN_NONZERO <= gfx_b <= METATILE_GFX_MAX):
                print(
                    f"index {i}: metatile_gfx_bytes must be 0 or aligned and in "
                    f"[{METATILE_GFX_MIN_NONZERO}, {METATILE_GFX_MAX}]",
                    file=sys.stderr,
                )
                return 1
        if attr_b != 0:
            if attr_b % METATILE_ATTR_UNIT != 0 or not (METATILE_ATTR_MIN_NONZERO <= attr_b <= METATILE_ATTR_MAX):
                print(
                    f"index {i}: metatile_attr_bytes must be 0 or aligned and in "
                    f"[{METATILE_ATTR_MIN_NONZERO}, {METATILE_ATTR_MAX}]",
                    file=sys.stderr,
                )
                return 1
        if pal_b != 0:
            if pal_b < VANILLA_PALETTE_BUNDLE or pal_b % 32 != 0:
                print(f"index {i}: palette_bundle_bytes invalid", file=sys.stderr)
                return 1
            if pal_b > PALETTE_BUNDLE_MAX:
                print(f"index {i}: palette_bundle_bytes exceeds max {PALETTE_BUNDLE_MAX}", file=sys.stderr)
                return 1
        if tile_u == 0:
            continue
        if tile_u < 32 or tile_u % 32 != 0:
            print(f"index {i}: tile_uncomp_bytes must be 0 or >=32 and multiple of 32", file=sys.stderr)
            return 1
        if tile_u > TILE_UNCOMP_MAX:
            print(
                f"index {i}: tile_uncomp_bytes {tile_u} exceeds MAP_BG field cap {TILE_UNCOMP_MAX} "
                f"(MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES)",
                file=sys.stderr,
            )
            return 1
        if role_caps is not None and tile_u > role_caps[i]:
            sym = role_symbols[i] if role_symbols else "?"
            print(
                f"index {i} ({sym}): tile_uncomp_bytes {tile_u} exceeds role cap {role_caps[i]} "
                f"(use --enforce-role-tile-caps with matching headers/table, or loosen caps)",
                file=sys.stderr,
            )
            return 1

    mode = "extended 16-byte" if row_stride == ROW_EXT else "legacy 8-byte"
    extra = " + role tile caps" if role_caps is not None else ""
    print(f"OK: compiled tileset asset profile valid ({args.count} rows, {mode}{extra}).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
