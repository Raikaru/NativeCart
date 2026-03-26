#!/usr/bin/env python3
"""
Structural offline validation for FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF.

Mirrors try_apply_field_map_palette_policy_from_rom in
firered_portable_rom_map_layout_metatiles.c (word layout only).

Usage:
  python validate_field_map_palette_policy.py <rom.gba> --off 0x...

See docs/portable_rom_map_layout_metatiles.md § Field map BG palette row split.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

NUM_PALS_TOTAL = 13


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to one little-endian u32 policy word")
    args = ap.parse_args()

    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or (off & 3) != 0:
        print("offset must be non-negative and 4-byte aligned", file=sys.stderr)
        return 1
    if off + 4 > len(rom):
        print("u32 policy word does not fit in ROM at given offset", file=sys.stderr)
        return 1

    (w,) = struct.unpack_from("<I", rom, off)
    if w == 0:
        print("OK: word 0 — vanilla split (7 primary / 6 secondary).")
        return 0

    pri = w & 0xFF
    sec = (w >> 8) & 0xFF
    hi = w >> 16
    if hi != 0:
        print(f"invalid: upper 16 bits must be 0 (got 0x{hi:04x})", file=sys.stderr)
        return 1
    if pri < 1 or sec < 1:
        print("invalid: primary and secondary row counts must be >= 1", file=sys.stderr)
        return 1
    if pri + sec != NUM_PALS_TOTAL:
        print(f"invalid: primary ({pri}) + secondary ({sec}) must equal {NUM_PALS_TOTAL}", file=sys.stderr)
        return 1

    print(f"OK: policy word 0x{w:08x} — {pri} primary rows, {sec} secondary rows.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
