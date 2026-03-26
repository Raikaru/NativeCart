#!/usr/bin/env python3
"""
Offline structural validation for FIRERED_ROM_FACILITY_CLASS_LOOKUPS_PACK_OFF (FCLC v1).

Mirrors firered_portable_rom_facility_class_lookups.c.

Vanilla: NUM_FACILITY_CLASSES = 150, pic byte < 148, trainer-class byte < 107.

Usage:
  python validate_facility_class_lookups_rom_pack.py <rom.gba> --off 0x...
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

MAGIC = 0x46434C43  # b'FCLC' LE
VERSION = 1
HEADER = 16

# Vanilla caps from include/constants/trainers.h
DEFAULT_ENTRY_COUNT = 150
DEFAULT_PIC_MAX_EXCL = 148  # TRAINER_PIC_PAINTER + 1
DEFAULT_CLASS_MAX_EXCL = 107  # TRAINER_CLASS_PAINTER + 1


def parse_hex(s: str) -> int:
    s = s.strip().replace("_", "")
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    ap.add_argument("--off", required=True, help="hex file offset to pack start")
    ap.add_argument("--entry-count", type=int, default=DEFAULT_ENTRY_COUNT, metavar="N")
    ap.add_argument("--pic-max-excl", type=int, default=DEFAULT_PIC_MAX_EXCL, metavar="N")
    ap.add_argument("--class-max-excl", type=int, default=DEFAULT_CLASS_MAX_EXCL, metavar="N")
    args = ap.parse_args()

    if args.entry_count < 1:
        print("entry-count must be >= 1", file=sys.stderr)
        return 2

    need = HEADER + 2 * args.entry_count
    rom = args.rom.read_bytes()
    off = parse_hex(args.off)
    if off < 0 or need > len(rom) or off > len(rom) - need:
        print("FCLC pack does not fit in ROM at given offset", file=sys.stderr)
        return 1

    magic, version, entry_count, reserved = struct.unpack_from("<4I", rom, off)
    if magic != MAGIC:
        print(f"bad magic 0x{magic:08x} (expected 0x{MAGIC:08x})", file=sys.stderr)
        return 1
    if version != VERSION:
        print(f"bad version {version}", file=sys.stderr)
        return 1
    if reserved != 0:
        print(f"reserved must be 0, got {reserved}", file=sys.stderr)
        return 1
    if entry_count != args.entry_count:
        print(f"entry_count mismatch: blob has {entry_count}, expected {args.entry_count}", file=sys.stderr)
        return 1

    pic = rom[off + HEADER : off + HEADER + args.entry_count]
    tc = rom[off + HEADER + args.entry_count : off + HEADER + 2 * args.entry_count]

    for i, b in enumerate(pic):
        if b >= args.pic_max_excl:
            print(f"pic row {i}: value 0x{b:02x} >= pic max exclusive {args.pic_max_excl}", file=sys.stderr)
            return 1
    for i, b in enumerate(tc):
        if b >= args.class_max_excl:
            print(f"trainer_class row {i}: value 0x{b:02x} >= class max exclusive {args.class_max_excl}", file=sys.stderr)
            return 1

    print(f"OK: FCLC pack valid ({args.entry_count} rows).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
