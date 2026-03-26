#!/usr/bin/env python3
"""
Offline structural validation for FIRERED_ROM_TYPE_EFFECTIVENESS_OFF.

Mirrors type_effectiveness_buf_valid in firered_portable_rom_type_effectiveness_table.c.

Vanilla: FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT = 336 bytes, triplets (atk, def, mul).

Usage:
  python validate_type_effectiveness_rom_table.py <rom.gba> --off 0x...
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

BYTE_COUNT = 336
NUMBER_OF_MON_TYPES = 18
TYPE_FORESIGHT = 0xFE
TYPE_ENDTABLE = 0xFF
TYPE_MUL_NO_EFFECT = 0
TYPE_MUL_NOT_EFFECTIVE = 5
TYPE_MUL_NORMAL = 10
TYPE_MUL_SUPER_EFFECTIVE = 20

VALID_MUL = {
    TYPE_MUL_NO_EFFECT,
    TYPE_MUL_NOT_EFFECTIVE,
    TYPE_MUL_NORMAL,
    TYPE_MUL_SUPER_EFFECTIVE,
}


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def buf_valid(buf: bytes) -> bool:
    if len(buf) != BYTE_COUNT:
        return False
    if buf[BYTE_COUNT - 3] != TYPE_ENDTABLE or buf[BYTE_COUNT - 2] != TYPE_ENDTABLE:
        return False
    i = 0
    while i + 2 < BYTE_COUNT:
        a, d, m = buf[i], buf[i + 1], buf[i + 2]
        if a == TYPE_ENDTABLE:
            if i != BYTE_COUNT - 3:
                return False
            return m in VALID_MUL
        if m not in VALID_MUL:
            return False
        if a == TYPE_FORESIGHT:
            if d != TYPE_FORESIGHT:
                return False
        else:
            if a >= NUMBER_OF_MON_TYPES or d >= NUMBER_OF_MON_TYPES:
                return False
        i += 3
    return False


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to 336-byte table start")
    args = ap.parse_args()

    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or BYTE_COUNT > len(rom) or off > len(rom) - BYTE_COUNT:
        print("type effectiveness table does not fit in ROM at given offset", file=sys.stderr)
        return 1

    buf = rom[off : off + BYTE_COUNT]
    if not buf_valid(buf):
        print("type effectiveness table failed structural validation", file=sys.stderr)
        return 1

    print(f"OK: type effectiveness table valid ({BYTE_COUNT} bytes).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
