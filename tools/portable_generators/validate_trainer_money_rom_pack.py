#!/usr/bin/env python3
"""
Structural offline validation for FIRERED_ROM_TRAINER_MONEY_TABLE_OFF
(`struct TrainerMoney`: u8 classId, u8 value; 0xFF terminator row).

Mirrors `trainer_money_rom_valid` in firered_portable_rom_trainer_money_table.c.

Usage:
  python validate_trainer_money_rom_pack.py <rom.gba> --off 0x...

See docs/portable_rom_trainer_money_table.md.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

MAX_ROWS = 256


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to first TrainerMoney row")
    args = ap.parse_args()

    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    need = MAX_ROWS * 2
    if off < 0 or need > len(rom) or off > len(rom) - need:
        print("trainer money table (256 rows) does not fit in ROM at given offset", file=sys.stderr)
        return 1

    for i in range(MAX_ROWS):
        base = off + i * 2
        class_id = rom[base]
        value = rom[base + 1]
        if class_id == 0xFF:
            if i == 0:
                print("terminator cannot be the only row (runtime requires >= 2 rows)", file=sys.stderr)
                return 1
            if value == 0:
                print(f"row {i}: terminator value must be non-zero", file=sys.stderr)
                return 1
            print(f"OK: trainer money pack valid, {i + 1} rows (including terminator).")
            return 0
        if value == 0:
            print(f"row {i}: non-terminator value must be non-zero", file=sys.stderr)
            return 1

    print("no 0xFF classId terminator within 256 rows", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
