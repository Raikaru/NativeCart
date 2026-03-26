#!/usr/bin/env python3
"""
Offline structural validation for FIRERED_ROM_MOVE_NAMES_PACK_OFF (MNMV v1).

Mirrors firered_portable_rom_move_names_table.c (header + EOS per row in payload).

Vanilla: MOVES_COUNT=355, MOVE_NAME_LENGTH+1=13, magic 0x564D4E4D.

Usage:
  python validate_move_names_rom_pack.py <rom.gba> --off 0x...
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

MAGIC = 0x564D4E4D  # MNMV LE
VERSION = 1
HEADER = 16
DEFAULT_ROWS = 355
DEFAULT_ROW_BYTES = 13
EOS = 0xFF


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def row_has_eos(row: bytes) -> bool:
    return EOS in row


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to pack start")
    ap.add_argument("--rows", type=int, default=DEFAULT_ROWS, metavar="N")
    ap.add_argument("--row-bytes", type=int, default=DEFAULT_ROW_BYTES, metavar="N")
    args = ap.parse_args()

    if args.rows < 1 or args.row_bytes < 1:
        print("rows and row-bytes must be positive", file=sys.stderr)
        return 2

    need = HEADER + args.rows * args.row_bytes
    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or need > len(rom) or off > len(rom) - need:
        print("move names pack does not fit in ROM at given offset", file=sys.stderr)
        return 1

    magic, version, rows, row_bytes = struct.unpack_from("<4I", rom, off)
    if magic != MAGIC:
        print(f"bad magic 0x{magic:08x} (expected 0x{MAGIC:08x})", file=sys.stderr)
        return 1
    if version != VERSION:
        print(f"bad version {version}", file=sys.stderr)
        return 1
    if rows != args.rows or row_bytes != args.row_bytes:
        print(
            f"header mismatch: rows={rows} row_bytes={row_bytes}, expected rows={args.rows} row_bytes={args.row_bytes}",
            file=sys.stderr,
        )
        return 1

    payload = rom[off + HEADER : off + HEADER + args.rows * args.row_bytes]
    for r in range(args.rows):
        row = payload[r * args.row_bytes : (r + 1) * args.row_bytes]
        if not row_has_eos(row):
            print(f"row {r}: no EOS (0xFF) within row_bytes", file=sys.stderr)
            return 1

    print(f"OK: move names pack valid (MNMV v1, {args.rows}×{args.row_bytes}).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
