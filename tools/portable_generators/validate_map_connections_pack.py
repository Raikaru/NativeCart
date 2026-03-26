#!/usr/bin/env python3
"""
Structural validation of a map-connections pack (dense grid).

Mirrors firered_portable_rom_map_connections.c validate_pack (minus gMapGroups
NULL checks — needs a built game / loaded ROM):

  - grid size MAP_GROUPS_COUNT * 256 * 100
  - unused slots (optional --map-groups): entire row must be 100 zero bytes
  - used rows: u32 head — count in low 8 bits, upper bits zero; count <= 8
  - active slots: MapConnection wire (direction 1..6, pads, LE offset, group/num, pad)
  - inactive slots: 12 zero bytes each

Usage:
  python validate_map_connections_pack.py <pack.bin> [--map-groups path/to/map_groups.json]
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

MAP_GROUPS_COUNT = 43
MAPNUM_MAX = 256
MAX_CONN = 8
ROW_BYTES = 4 + MAX_CONN * 12
GRID_BYTES = MAP_GROUPS_COUNT * MAPNUM_MAX * ROW_BYTES

DIR_MIN = 1
DIR_MAX = 6


def read_le_u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def validate_slot(slot: bytes, active: bool, group_sizes: list[int] | None) -> None:
    if len(slot) != 12:
        raise ValueError("internal: bad slot length")
    if not active:
        if slot != b"\x00" * 12:
            raise ValueError("inactive slot must be 12 zero bytes")
        return
    direction = slot[0]
    if direction < DIR_MIN or direction > DIR_MAX:
        raise ValueError(f"bad direction {direction}")
    if slot[1:4] != b"\x00\x00\x00":
        raise ValueError("direction pad non-zero")
    # offset u32 at +4 — any pattern allowed (signed s32 wire)
    mg = slot[8]
    mn = slot[9]
    if slot[10:12] != b"\x00\x00":
        raise ValueError("tail pad non-zero")
    if mg >= MAP_GROUPS_COUNT:
        raise ValueError(f"mapGroup out of range {mg}")
    if group_sizes is not None:
        if mn >= group_sizes[mg]:
            raise ValueError(f"mapNum out of range for group {mg}: {mn}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("pack", type=Path)
    ap.add_argument("--map-groups", type=Path, default=None)
    args = ap.parse_args()
    data = args.pack.read_bytes()
    if len(data) < GRID_BYTES:
        raise SystemExit(f"file too small (need >= {GRID_BYTES} for grid)")

    map_groups = None
    if args.map_groups:
        map_groups = json.loads(args.map_groups.read_text(encoding="utf-8"))

    group_sizes: list[int] | None = None
    if map_groups:
        group_sizes = [
            len(map_groups[gname])
            for gname in map_groups["group_order"]
        ]
        while len(group_sizes) < MAP_GROUPS_COUNT:
            group_sizes.append(0)

    zrow = b"\x00" * ROW_BYTES

    for g in range(MAP_GROUPS_COUNT):
        maxn = MAPNUM_MAX
        if map_groups and g < len(map_groups["group_order"]):
            maxn = len(map_groups[map_groups["group_order"][g]])
        for n in range(MAPNUM_MAX):
            idx = (g * MAPNUM_MAX + n) * ROW_BYTES
            row = data[idx : idx + ROW_BYTES]
            if n >= maxn:
                if row != zrow:
                    raise SystemExit(f"non-zero unused row group={g} num={n}")
                continue

            head = read_le_u32(data, idx)
            count = head & 0xFF
            if (head & ~0xFF) != 0:
                raise SystemExit(f"bad head reserved bits group={g} num={n}")
            if count > MAX_CONN:
                raise SystemExit(f"count too large group={g} num={n}: {count}")

            for i in range(MAX_CONN):
                slot = data[idx + 4 + i * 12 : idx + 4 + i * 12 + 12]
                try:
                    validate_slot(slot, i < count, group_sizes)
                except ValueError as e:
                    raise SystemExit(f"group={g} num={n} slot={i}: {e}") from e

    print("OK", args.pack, f"({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
