#!/usr/bin/env python3
"""
Structural validation of a WINF wild-encounter family pack (embedded slice or standalone file).

Mirrors firered_portable_rom_wild_encounter_family.c (refresh + validate path):
  - magic 0x464E4957 ('WINF'), version 1, header_bytes
  - GBA-wire header rows: 20 bytes (mapGroup, mapNum, 2 pad zeros, 4 LE u32 pointers)
  - non-NULL pointers: 0x08…… in-file; WildPokemonInfo (8 bytes) + WildPokemon slot tables
  - last header row must be MAP_UNDEFINED (0xFF, 0xFF) with zero pointers; no rows after

Does not prove gWildMonHeaders / altering-cave row adjacency vs your compiled game.

Usage:
  python validate_wild_encounter_family_pack.py <rom_or_pack.bin> [--pack-offset N] [--max-species M]
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

MAGIC = 0x464E4957  # WINF LE
VERSION = 1
HEADER_PREFIX = 12
HEADER_ROW = 20
INFO_BYTES = 8
MON_BYTES = 4
LAND = 12
WATER = 5
ROCK = 5
FISH = 10

MAP_UNDEFINED_GROUP = 0xFF
MAP_UNDEFINED_NUM = 0xFF


def read_le_u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def gba_ptr_to_off(gba: int, rom_len: int) -> int | None:
    if gba == 0:
        return None
    if gba < 0x08000000 or gba >= 0x0A000000:
        return None
    off = gba - 0x08000000
    if off >= rom_len:
        return None
    return off


def species_ok(sp: int, max_species: int) -> bool:
    if sp == 0:
        return True
    return 1 <= sp <= max_species


def validate_info_chain(data: bytes, rom_len: int, info_gba: int, slot_count: int, max_species: int) -> None:
    ip_off = gba_ptr_to_off(info_gba, rom_len)
    if ip_off is None:
        raise ValueError("bad WildPokemonInfo GBA pointer")
    if ip_off + INFO_BYTES > rom_len:
        raise ValueError("WildPokemonInfo OOB")
    mon_gba = read_le_u32(data, ip_off + 4)
    mp_off = gba_ptr_to_off(mon_gba, rom_len)
    if mp_off is None:
        raise ValueError("bad WildPokemon[] GBA pointer")
    need = slot_count * MON_BYTES
    if mp_off + need > rom_len:
        raise ValueError("WildPokemon[] OOB")
    for j in range(slot_count):
        base = mp_off + j * MON_BYTES
        species = struct.unpack_from("<H", data, base + 2)[0]
        if not species_ok(species, max_species):
            raise ValueError(f"bad species {species} at slot {j}")


def validate_header_row(
    data: bytes, rom_len: int, row_off: int, max_species: int
) -> None:
    if row_off + HEADER_ROW > rom_len:
        raise ValueError("header row OOB")
    row = data[row_off : row_off + HEADER_ROW]
    if row[2] != 0 or row[3] != 0:
        raise ValueError("header padding bytes must be zero")
    land = read_le_u32(data, row_off + 4)
    water = read_le_u32(data, row_off + 8)
    rock = read_le_u32(data, row_off + 12)
    fish = read_le_u32(data, row_off + 16)
    if land != 0:
        validate_info_chain(data, rom_len, land, LAND, max_species)
    if water != 0:
        validate_info_chain(data, rom_len, water, WATER, max_species)
    if rock != 0:
        validate_info_chain(data, rom_len, rock, ROCK, max_species)
    if fish != 0:
        validate_info_chain(data, rom_len, fish, FISH, max_species)


def is_sentinel_row(data: bytes, row_off: int) -> bool:
    return data[row_off] == MAP_UNDEFINED_GROUP and data[row_off + 1] == MAP_UNDEFINED_NUM


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("path", type=Path)
    ap.add_argument("--pack-offset", type=int, default=0, help="byte offset of WINF header in file")
    ap.add_argument("--max-species", type=int, default=412, help="NUM_SPECIES for your tree (vanilla FR ~412)")
    args = ap.parse_args()
    data = args.path.read_bytes()
    rom_len = len(data)
    po = args.pack_offset
    if po < 0 or po + HEADER_PREFIX > rom_len:
        raise SystemExit("pack offset OOB")

    magic = read_le_u32(data, po)
    version = read_le_u32(data, po + 4)
    header_bytes = read_le_u32(data, po + 8)
    if magic != MAGIC:
        raise SystemExit(f"bad magic 0x{magic:08X}")
    if version != VERSION:
        raise SystemExit(f"bad version {version}")
    if header_bytes == 0 or header_bytes % HEADER_ROW != 0:
        raise SystemExit("bad header_bytes")

    blob_start = po + HEADER_PREFIX
    if blob_start > rom_len or header_bytes > rom_len - blob_start:
        raise SystemExit("header blob OOB")

    pos = 0
    while pos < header_bytes:
        row_off = blob_start + pos
        try:
            validate_header_row(data, rom_len, row_off, args.max_species)
        except ValueError as e:
            raise SystemExit(f"row pos={pos}: {e}") from e
        if is_sentinel_row(data, row_off):
            if pos + HEADER_ROW != header_bytes:
                raise SystemExit("MAP_UNDEFINED sentinel must be the final header row")
            if read_le_u32(data, row_off + 4) != 0 or read_le_u32(data, row_off + 8) != 0:
                raise SystemExit("sentinel row pointers must be zero")
            if read_le_u32(data, row_off + 12) != 0 or read_le_u32(data, row_off + 16) != 0:
                raise SystemExit("sentinel row pointers must be zero")
            print("OK", args.path, f"(pack_offset=0x{po:X}, header_bytes={header_bytes})")
            return 0
        pos += HEADER_ROW

    raise SystemExit("no MAP_UNDEFINED terminator row")


if __name__ == "__main__":
    raise SystemExit(main())
