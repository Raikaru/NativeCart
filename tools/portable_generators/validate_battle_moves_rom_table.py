#!/usr/bin/env python3
"""
Offline structural validation for FIRERED_ROM_BATTLE_MOVES_TABLE_OFF.

Mirrors battle_moves_rom_valid in firered_portable_rom_battle_moves_table.c.

Vanilla: MOVES_COUNT=355, 9 bytes per struct BattleMove row.

Usage:
  python validate_battle_moves_rom_table.py <rom.gba> --off 0x...
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

ROW_BYTES = 9
DEFAULT_MOVES_COUNT = 355
NUMBER_OF_MON_TYPES = 18
EFFECT_HIT = 0
EFFECT_CAMOUFLAGE = 213
TYPE_NORMAL = 0


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def row_valid(effect, power, type_, accuracy, pp, sec, target, priority, flags) -> bool:
    if type_ >= NUMBER_OF_MON_TYPES:
        return False
    if accuracy > 100:
        return False
    if pp > 80:
        return False
    if sec > 100:
        return False
    if target > 0x7F:
        return False
    if effect > EFFECT_CAMOUFLAGE:
        return False
    return True


def move_none_row_valid(effect, power, type_, accuracy, pp, sec, target, priority, flags) -> bool:
    return (
        effect == EFFECT_HIT
        and power == 0
        and type_ == TYPE_NORMAL
        and accuracy == 0
        and pp == 0
        and sec == 0
        and target == 0
        and priority == 0
        and flags == 0
    )


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to first BattleMove row")
    ap.add_argument("--moves-count", type=int, default=DEFAULT_MOVES_COUNT, metavar="N")
    args = ap.parse_args()

    if args.moves_count < 1:
        print("moves-count must be positive", file=sys.stderr)
        return 2

    need = args.moves_count * ROW_BYTES
    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or need > len(rom) or off > len(rom) - need:
        print("battle moves table does not fit in ROM at given offset", file=sys.stderr)
        return 1

    for i in range(args.moves_count):
        chunk = rom[off + i * ROW_BYTES : off + (i + 1) * ROW_BYTES]
        effect, power, type_, accuracy, pp, sec, target, priority, flags = struct.unpack(
            "<7BbB", chunk
        )
        if not row_valid(effect, power, type_, accuracy, pp, sec, target, priority, flags):
            print(f"row {i}: failed field-range validation", file=sys.stderr)
            return 1

    chunk0 = rom[off : off + ROW_BYTES]
    effect, power, type_, accuracy, pp, sec, target, priority, flags = struct.unpack("<7BbB", chunk0)
    if not move_none_row_valid(effect, power, type_, accuracy, pp, sec, target, priority, flags):
        print("row 0 (MOVE_NONE): does not match vanilla empty-move contract", file=sys.stderr)
        return 1

    print(f"OK: battle moves table valid ({args.moves_count}×{ROW_BYTES} bytes).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
