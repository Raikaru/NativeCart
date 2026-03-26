#!/usr/bin/env python3
"""
Emit a WINF wild-encounter family pack matching firered_portable_rom_wild_encounter_family.c.

Source: src/data/wild_encounters.json (group label gWildMonHeaders), filtered by edition
(FireRed vs LeafGreen via base_label suffix). Map ids and species come from generated
include/constants/map_groups.h and include/constants/species.h.

Pointer encoding: each GBA pointer is 0x08000000 + file_offset_base + offset_within_emitted_bin.
Default file_offset_base=0 validates as a standalone .bin with validate_wild_encounter_family_pack.py.
When embedding the raw pack bytes at offset P in a full .gba, pass --file-offset-base P.

Usage:
  python build_wild_encounter_family_pack.py <pokefirered_root> <output.bin> \\
      [--edition firered|leafgreen] [--file-offset-base N]
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

MAGIC = 0x464E4957
VERSION = 1
HEADER_PREFIX = 12
HEADER_ROW = 20
LAND = 12
WATER = 5
ROCK = 5
FISH = 10

ROM_LO = 0x08000000
ROM_HI = 0x0A000000

MAP_RE = re.compile(
    r"^#define\s+(MAP_\w+)\s+\(\s*(\d+)\s*\|\s*\(\s*(\d+)\s*<<\s*8\s*\)\s*\)\s*$"
)
SPECIES_RE = re.compile(r"^#define\s+(SPECIES_\w+)\s+(\d+)\s*$")


def load_map_constants(map_groups_h: Path) -> dict[str, tuple[int, int]]:
    out: dict[str, tuple[int, int]] = {}
    for line in map_groups_h.read_text(encoding="utf-8", errors="replace").splitlines():
        m = MAP_RE.match(line.strip())
        if not m:
            continue
        name, num_s, group_s = m.group(1), m.group(2), m.group(3)
        out[name] = (int(group_s), int(num_s))
    return out


def load_species_constants(species_h: Path) -> dict[str, int]:
    out: dict[str, int] = {}
    for line in species_h.read_text(encoding="utf-8", errors="replace").splitlines():
        m = SPECIES_RE.match(line.strip())
        if not m:
            continue
        out[m.group(1)] = int(m.group(2))
    return out


def gba_ptr(file_offset_base: int, off_in_bin: int) -> int:
    v = ROM_LO + file_offset_base + off_in_bin
    if v >= ROM_HI:
        raise ValueError(f"GBA pointer overflow at off_in_bin={off_in_bin}")
    return v


def append_info_and_slots(
    buf: bytearray,
    file_offset_base: int,
    encounter_rate: int,
    mons: list,
    slot_count: int,
    species: dict[str, int],
    label: str,
    which: str,
) -> int:
    if len(mons) != slot_count:
        raise ValueError(f"{label} {which}: expected {slot_count} mons, got {len(mons)}")
    info_off = len(buf)
    buf.extend(b"\x00" * 8)
    mon_off = len(buf)
    for i, m in enumerate(mons):
        spn = m["species"]
        if spn not in species:
            raise ValueError(f"{label} {which} slot {i}: unknown species {spn!r}")
        buf.extend(
            struct.pack(
                "<BBH",
                int(m["min_level"]),
                int(m["max_level"]),
                species[spn],
            )
        )
    mon_gba = gba_ptr(file_offset_base, mon_off)
    struct.pack_into("<B3xI", buf, info_off, encounter_rate & 0xFF, mon_gba)
    return gba_ptr(file_offset_base, info_off)


def ptr_for_encounter(
    buf: bytearray,
    file_offset_base: int,
    enc: dict,
    key: str,
    slot_count: int,
    species: dict[str, int],
) -> int:
    block = enc.get(key)
    if not block:
        return 0
    label = enc.get("map", enc.get("base_label", "?"))
    return append_info_and_slots(
        buf,
        file_offset_base,
        int(block["encounter_rate"]),
        block["mons"],
        slot_count,
        species,
        str(label),
        key,
    )


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("root", type=Path, help="pokefirered repo root")
    ap.add_argument("output", type=Path)
    ap.add_argument(
        "--edition",
        choices=("firered", "leafgreen"),
        default="firered",
        help="which game's encounter rows to emit (matches JSON base_label suffix)",
    )
    ap.add_argument(
        "--file-offset-base",
        type=int,
        default=0,
        help="byte offset where this pack starts inside the final .gba (0 for standalone .bin test)",
    )
    args = ap.parse_args()
    root = args.root.resolve()
    suffix = "_FireRed" if args.edition == "firered" else "_LeafGreen"

    wild_json = root / "src" / "data" / "wild_encounters.json"
    map_h = root / "include" / "constants" / "map_groups.h"
    species_h = root / "include" / "constants" / "species.h"

    data = json.loads(wild_json.read_text(encoding="utf-8"))
    maps = load_map_constants(map_h)
    species = load_species_constants(species_h)

    group = None
    for g in data.get("wild_encounter_groups", []):
        if g.get("label") == "gWildMonHeaders":
            group = g
            break
    if group is None:
        raise SystemExit("wild_encounters.json: missing wild_encounter_groups label gWildMonHeaders")

    encounters_raw = group.get("encounters", [])
    encounters = [e for e in encounters_raw if str(e.get("base_label", "")).endswith(suffix)]
    if not encounters:
        raise SystemExit(f"no encounters for edition suffix {suffix!r}")

    n = len(encounters)
    header_bytes = (n + 1) * HEADER_ROW
    buf = bytearray(HEADER_PREFIX + header_bytes)

    for i, enc in enumerate(encounters):
        mname = enc.get("map")
        if not mname or mname not in maps:
            raise SystemExit(f"encounter {i}: bad or unknown map {mname!r}")
        mg, mn = maps[mname]
        land = ptr_for_encounter(buf, args.file_offset_base, enc, "land_mons", LAND, species)
        water = ptr_for_encounter(buf, args.file_offset_base, enc, "water_mons", WATER, species)
        rock = ptr_for_encounter(buf, args.file_offset_base, enc, "rock_smash_mons", ROCK, species)
        fish = ptr_for_encounter(buf, args.file_offset_base, enc, "fishing_mons", FISH, species)
        struct.pack_into(
            "<BBxxIIII",
            buf,
            HEADER_PREFIX + i * HEADER_ROW,
            mg & 0xFF,
            mn & 0xFF,
            land,
            water,
            rock,
            fish,
        )

    struct.pack_into("<BBxxIIII", buf, HEADER_PREFIX + n * HEADER_ROW, 0xFF, 0xFF, 0, 0, 0, 0)
    struct.pack_into("<III", buf, 0, MAGIC, VERSION, header_bytes)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(buf)
    print(f"Wrote {len(buf)} bytes ({n} maps + sentinel) to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
