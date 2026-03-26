#!/usr/bin/env python3
"""
Build a deterministic **offline** test ROM: layout companion splice output + minimal intro-prose table.

**Input:** ``build/offline_splice_test_out.bin`` (from ``layout_rom_companion_prepare_test_rom.py`` /
``tools/run_offline_build_gates.py``).

**Output:** ``build/offline_layout_prose_fixture.bin`` — same layout pack prefix, then three
single-byte strings (``A``/``B``/``C`` + EOS) and a 3×``u32`` little-endian pointer table at EOF.

**CRC32:** IEEE polynomial, full image, matching ``firered_portable_rom_crc32_ieee_full()`` in
``firered_portable_rom_queries.c``. Keep the golden constants here aligned with:

- ``cores/firered/portable/firered_portable_rom_map_layout_metatiles.c`` (layout directory profile rows)
- ``cores/firered/portable/firered_portable_new_game_intro_prose_rom.c`` (intro prose profile row)
- ``cores/firered/portable/firered_portable_rom_compat.c`` (optional ``KnownProfileRow`` labels)

When the companion bundle or splice host changes, this script will fail its CRC check — regenerate
the golden values in those files.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Golden IEEE CRC32s (must match C sources listed in the module docstring).
OFFLINE_SPLICE_EXPECT_CRC32 = 0x33B8FD06
OFFLINE_LAYOUT_PROSE_FIXTURE_EXPECT_CRC32 = 0x217A04B1

ROM_BASE = 0x08000000


def crc32_ieee(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 & (0xFFFFFFFF if (crc & 1) else 0))
    return crc ^ 0xFFFFFFFF


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo-root", type=Path, default=None, help="repo root (default: cwd)")
    ap.add_argument(
        "--input",
        type=Path,
        default=None,
        help="layout splice output (default: <root>/build/offline_splice_test_out.bin)",
    )
    ap.add_argument(
        "--output",
        type=Path,
        default=None,
        help="written fixture (default: <root>/build/offline_layout_prose_fixture.bin)",
    )
    args = ap.parse_args()

    root = (args.repo_root if args.repo_root is not None else Path.cwd()).resolve()
    inp = (args.input if args.input is not None else root / "build" / "offline_splice_test_out.bin").resolve()
    out = (args.output if args.output is not None else root / "build" / "offline_layout_prose_fixture.bin").resolve()

    if not inp.is_file():
        print(f"error: missing input {inp}", file=sys.stderr)
        return 1

    base = inp.read_bytes()
    got = crc32_ieee(base)
    if got != OFFLINE_SPLICE_EXPECT_CRC32:
        print(
            f"error: input CRC 0x{got:08X} != expected 0x{OFFLINE_SPLICE_EXPECT_CRC32:08X} "
            f"(update goldens or rebuild splice ROM)",
            file=sys.stderr,
        )
        return 1

    strings = bytes([ord("A"), 0xFF, ord("B"), 0xFF, ord("C"), 0xFF])
    base_len = len(base)
    offs = [base_len, base_len + 2, base_len + 4]
    rom2 = base + strings
    table = b"".join((ROM_BASE + o).to_bytes(4, "little") for o in offs)
    fixture = rom2 + table

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(fixture)

    fcrc = crc32_ieee(fixture)
    if fcrc != OFFLINE_LAYOUT_PROSE_FIXTURE_EXPECT_CRC32:
        print(
            f"error: fixture CRC 0x{fcrc:08X} != expected 0x{OFFLINE_LAYOUT_PROSE_FIXTURE_EXPECT_CRC32:08X}",
            file=sys.stderr,
        )
        return 1

    print(f"Wrote {out} (bytes={len(fixture):#x}) crc32=0x{fcrc:08X}")
    print(f"  prose table_off=0x{len(rom2):X} page_count=3")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
