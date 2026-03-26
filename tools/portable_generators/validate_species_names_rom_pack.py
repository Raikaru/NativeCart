#!/usr/bin/env python3
"""
Offline structural validation for FIRERED_ROM_SPECIES_NAMES_PACK_OFF.
Mirrors row_has_eos / species_names_rom_valid in firered_portable_rom_species_names_pack.c.

Default grid: (SPECIES_CHIMECHO + 1) rows × (POKEMON_NAME_LENGTH + 1) bytes = 412 × 11 = 4532.

Usage:
  python validate_species_names_rom_pack.py <rom.gba> --off 0x...
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Vanilla FireRed portable snapshot (see docs/portable_rom_species_names_pack.md)
DEFAULT_ROWS = 412
DEFAULT_STRIDE = 11


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def row_has_eos(row: bytes) -> bool:
    return any(b == 0xFF for b in row)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to pack start")
    ap.add_argument("--rows", type=int, default=DEFAULT_ROWS, metavar="N")
    ap.add_argument("--stride", type=int, default=DEFAULT_STRIDE, metavar="N")
    args = ap.parse_args()

    if args.rows < 1 or args.stride < 1:
        print("rows and stride must be positive", file=sys.stderr)
        return 2

    need = args.rows * args.stride
    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or need > len(rom) or off > len(rom) - need:
        print("species names pack does not fit in ROM at given offset", file=sys.stderr)
        return 1

    for r in range(args.rows):
        row = rom[off + r * args.stride : off + (r + 1) * args.stride]
        if not row_has_eos(row):
            print(f"row {r}: no EOS (0xFF) within stride", file=sys.stderr)
            return 1

    print(f"OK: species names pack valid ({args.rows} rows × {args.stride} bytes).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
