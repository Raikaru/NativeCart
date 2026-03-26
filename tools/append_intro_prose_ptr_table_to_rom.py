#!/usr/bin/env python3
"""
Append a contiguous little-endian u32[] pointer table at EOF for the generic intro-prose seam
(`FIRERED_ROM_NEW_GAME_INTRO_PROSE_PTR_TABLE_OFF` + `FIRERED_ROM_NEW_GAME_INTRO_PROSE_PAGE_COUNT`).

Each `--string-off` is a **file offset** into the ROM image; the table stores `0x08000000 + off` per entry.
Validates EOS (0xFF) within the first 8192 bytes of each string (matches portable runtime cap).

Example (Fire Red Omega prose offsets from audit; use your own ROM copy paths):

  python tools/append_intro_prose_ptr_table_to_rom.py ^
    "Pokemon Fire Red Omega.gba" -o build/omega_intro_prose_test.gba ^
    --string-off 0x71A78D --string-off 0x71AA66 --string-off 0x71A972
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

MAX_STR = 8192
ROM_BASE = 0x08000000


def _eos_ok(rom: bytes, file_off: int):
    if file_off < 0 or file_off >= len(rom):
        return False, "string offset out of ROM range"
    window = rom[file_off : file_off + MAX_STR]
    if not window:
        return False, "empty window"
    try:
        window.index(0xFF)
    except ValueError:
        return False, f"no EOS (0xFF) within {MAX_STR} bytes"
    return True, ""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input_rom", type=Path, help="Source .gba (unmodified except prior patches)")
    ap.add_argument("-o", "--output", type=Path, required=True, help="Written copy with table appended")
    ap.add_argument(
        "--string-off",
        action="append",
        default=[],
        metavar="OFF",
        help="Hex or decimal file offset of one GBA-encoded string (repeat per page)",
    )
    args = ap.parse_args()
    if not args.string_off:
        ap.error("at least one --string-off required")

    rom = args.input_rom.read_bytes()
    offs: list[int] = []
    for s in args.string_off:
        o = int(s, 0)
        ok, err = _eos_ok(rom, o)
        if not ok:
            print(f"error: 0x{o:X}: {err}", file=sys.stderr)
            return 1
        offs.append(o)

    table = b"".join((ROM_BASE + o).to_bytes(4, "little") for o in offs)
    table_off = len(rom)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(rom + table)

    print(f"Wrote {args.output} (original_size=0x{table_off:X} appended_bytes={len(table)})")
    print(f"FIRERED_ROM_NEW_GAME_INTRO_PROSE_PTR_TABLE_OFF=0x{table_off:X}")
    print(f"FIRERED_ROM_NEW_GAME_INTRO_PROSE_PAGE_COUNT={len(offs)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
