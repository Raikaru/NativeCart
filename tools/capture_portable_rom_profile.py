#!/usr/bin/env python3
"""
Capture **ROM identity** and optional **seam offsets** for registering a manually tested ROM as a
portable **auto-bind profile**.

Matches runtime identity:
  - **CRC32:** IEEE, full file, same as ``firered_portable_rom_crc32_ieee_full()`` in
    ``cores/firered/portable/firered_portable_rom_queries.c``.
  - **Header:** game code @ **0xAC** (4 bytes), internal title @ **0xA0** (12 bytes), software
    version @ **0xBC** (1 byte) — same as ``firered_portable_rom_compat_refresh_after_rom_load``.

Outputs:
  1. Human-readable **identity** (for paste into issues / notes).
  2. **Candidate C snippets** for:
     - ``KnownProfileRow`` in ``firered_portable_rom_compat.c`` (CRC-first row is the usual choice
       for a unique built image; title/game-code rows are an alternative when CRC is unstable).
     - ``FireredRomU32TableProfileRow`` layout companion triple in
       ``firered_portable_rom_map_layout_metatiles.c`` (when ``--layout-*`` options are set).
     - ``FireredRomIntroProseProfileRow`` in ``firered_portable_new_game_intro_prose_rom.c`` (when
       ``--prose-*`` options are set).

**Env remains the escape hatch:** profile rows only apply when the matching env vars are unset.

Examples::

  python tools/capture_portable_rom_profile.py build/offline_splice_test_out.bin

  python tools/capture_portable_rom_profile.py build/offline_layout_prose_fixture.bin \\
    --profile-id my_hack --label "My hack (validated)" \\
    --layout-meta-off 0 --layout-dim-off 0x788DE --layout-ts-off 0x794D6 \\
    --prose-table-off 0x80006 --prose-pages 3
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


def crc32_ieee(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 & (0xFFFFFFFF if (crc & 1) else 0))
    return crc ^ 0xFFFFFFFF


def read_gba_header_fields(rom: bytes) -> tuple[bytes, bytes, int]:
    if len(rom) < 0xBD:
        return (b"", b"", 0)
    title = rom[0xA0 : 0xA0 + 12]
    game_code = rom[0xAC : 0xAC + 4]
    ver = rom[0xBC]
    return (title, game_code, ver)


def title_ascii(title12: bytes) -> str:
    parts: list[str] = []
    for c in title12:
        parts.append(chr(c) if 32 <= c <= 126 else ".")
    return "".join(parts).rstrip()


def c_str4(game_code: bytes) -> str:
    if len(game_code) < 4:
        return '""'
    esc = []
    for b in game_code:
        if b == ord("\\"):
            esc.append("\\\\")
        elif b == ord('"'):
            esc.append('\\"')
        elif 32 <= b <= 126:
            esc.append(chr(b))
        else:
            esc.append(f"\\x{b:02x}")
    return '"' + "".join(esc) + '"'


def parse_hex_int(s: str) -> int:
    return int(s.replace("_", ""), 0)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("rom", type=Path, help="Path to .gba / ROM image")
    ap.add_argument("--profile-id", default="my_rom_profile", help="slug for KnownProfileRow / notes")
    ap.add_argument("--label", default="TODO: human-readable label", help="display_label for KnownProfileRow")
    ap.add_argument(
        "--json",
        action="store_true",
        help="emit a single JSON object after the text report (stdout)",
    )
    ap.add_argument("--layout-meta-off", metavar="HEX", help="FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF")
    ap.add_argument("--layout-dim-off", metavar="HEX", help="FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF")
    ap.add_argument("--layout-ts-off", metavar="HEX", help="FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF")
    ap.add_argument("--prose-table-off", metavar="HEX", help="intro prose u32 pointer table file offset")
    ap.add_argument("--prose-pages", type=int, metavar="N", help="intro prose page count (1..16)")
    args = ap.parse_args()

    p = args.rom.resolve()
    if not p.is_file():
        print(f"error: not a file: {p}", file=sys.stderr)
        return 1

    rom = p.read_bytes()
    crc = crc32_ieee(rom)
    title12, gc4, sw_ver = read_gba_header_fields(rom)
    title_vis = title_ascii(title12)

    layout_meta = args.layout_meta_off
    layout_dim = args.layout_dim_off
    layout_ts = args.layout_ts_off
    prose_off = args.prose_table_off
    prose_n = args.prose_pages

    layout_parts = sum(x is not None for x in (layout_meta, layout_dim, layout_ts))
    if layout_parts not in (0, 3):
        print(
            "error: set all three of --layout-meta-off, --layout-dim-off, --layout-ts-off "
            "(or none for identity-only output).",
            file=sys.stderr,
        )
        return 2

    prose_parts = sum(x is not None for x in (prose_off, prose_n))
    if prose_parts == 1:
        print("error: set both --prose-table-off and --prose-pages, or neither.", file=sys.stderr)
        return 2
    if prose_n is not None and not (1 <= prose_n <= 16):
        print("error: --prose-pages must be 1 .. 16", file=sys.stderr)
        return 2

    meta_i = parse_hex_int(layout_meta) if layout_meta else None
    dim_i = parse_hex_int(layout_dim) if layout_dim else None
    ts_i = parse_hex_int(layout_ts) if layout_ts else None
    prose_i = parse_hex_int(prose_off) if prose_off else None

    print("=== Portable ROM profile capture ===")
    print(f"file: {p}")
    print(f"size: {len(rom)} (0x{len(rom):X})")
    print(f"CRC32 (IEEE, full image): 0x{crc:08X}  # must match firered_portable_rom_crc32_ieee_full()")
    print(f"game_code @0xAC: {gc4!r}  (as C string {c_str4(gc4)})")
    print(f"title     @0xA0: {title_vis!r}")
    print(f"software_version @0xBC: {sw_ver}")
    print()
    print("--- Suggested title_needle (for KnownProfileRow, if not using CRC-only) ---")
    print("Pick a unique ASCII substring that appears in the 12-byte internal title above;")
    print("compat uses strstr on a space-normalized copy (see firered_portable_rom_compat.c).")
    m = re.search(r"[A-Za-z0-9]{3,}", title_vis)
    if m:
        print(f"example needle from this ROM: {m.group(0)!r}")
    else:
        print("(no obvious alphanumeric run; choose manually)")
    print()

    crc_u = f"0x{crc:08X}u"
    pid = args.profile_id.replace('"', '\\"')
    lab = args.label.replace("\\", "\\\\").replace('"', '\\"')

    print("--- Candidate: KnownProfileRow (CRC-first; place before generic CRC=0 rows) ---")
    print("// firered_portable_rom_compat.c - s_known_profiles[]")
    print("{")
    print(f"    {crc_u},")
    print("    NULL,")
    print("    NULL,")
    print(f'    "{pid}",')
    print(f'    "{lab}",')
    print("    FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK,")
    print("    FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS")
    print("        | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,")
    print("},")
    print("// If this image is not BPRE/BPGE-shaped, use FIRERED_ROM_COMPAT_KIND_NONSTANDARD instead.")
    print()

    print("--- Alternative: KnownProfileRow (CRC wildcard + game code + title needle) ---")
    print("// Use when the same hack build is distributed with varying padding but stable header.")
    print("{")
    print("    0u,")
    print(f"    {c_str4(gc4)},")
    print('    "YOUR_UNIQUE_SUBSTRING",  // edit: must match internal title')
    print(f'    "{pid}",')
    print(f'    "{lab}",')
    print("    FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK,")
    print("    FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS")
    print("        | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,")
    print("},")
    print()

    if layout_parts == 3:
        assert meta_i is not None and dim_i is not None and ts_i is not None
        print("--- Candidate: layout companion FireredRomU32TableProfileRow (same CRC each) ---")
        print("// firered_portable_rom_map_layout_metatiles.c")
        print(f"    {{ {crc_u}, NULL, 0x{meta_i:X}u }},  // s_map_layout_metatiles_profile_rows")
        print(f"    {{ {crc_u}, NULL, 0x{dim_i:X}u }},  // s_map_layout_dimensions_profile_rows")
        print(f"    {{ {crc_u}, NULL, 0x{ts_i:X}u }},  // s_map_layout_tileset_indices_profile_rows")
        print("// Add matching rows to other layout-related profile tables if you use those seams.")
        print()

    if prose_parts == 2:
        assert prose_i is not None and prose_n is not None
        print("--- Candidate: FireredRomIntroProseProfileRow ---")
        print("// firered_portable_new_game_intro_prose_rom.c - s_intro_prose_profile_rows[]")
        print(f"    {{ {crc_u}, NULL, 0x{prose_i:X}u, {prose_n}u }},")
        print()

    if args.json:
        payload = {
            "path": str(p),
            "size": len(rom),
            "crc32_ieee_full": crc,
            "crc32_ieee_full_hex": f"0x{crc:08X}",
            "game_code": gc4.decode("latin-1", errors="replace"),
            "title_ascii": title_vis,
            "software_version": sw_ver,
            "profile_id_suggestion": args.profile_id,
            "label_suggestion": args.label,
            "layout": None
            if layout_parts == 0
            else {
                "metatiles_directory_off": meta_i,
                "dimensions_directory_off": dim_i,
                "tileset_indices_directory_off": ts_i,
            },
            "intro_prose": None
            if prose_parts == 0
            else {
                "ptr_table_off": prose_i,
                "page_count": prose_n,
            },
        }
        print("--- JSON ---")
        print(json.dumps(payload, indent=2))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
