#!/usr/bin/env python3
"""
Structural validation of a map-scripts directory image (grid only or grid + blob pool).

Mirrors firered_portable_rom_map_scripts_directory.c:
  - grid size MAP_GROUPS_COUNT * 256 * 8
  - unused slots (optional --map-groups) must be 8 zero bytes
  - 0/0 rows: no blob
  - non-0/0: blob must fit in file; tagged 5-byte walk must hit tag 0 before end (padding after 0 allowed)

Does not prove script pointer words resolve at runtime.

Usage:
  python validate_map_scripts_directory_pack.py <pack.bin> [--map-groups path/to/map_groups.json]
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

MAP_GROUPS_COUNT = 43
MAPNUM_MAX = 256
DIR_ROW_BYTES = 8
GRID_BYTES = MAP_GROUPS_COUNT * MAPNUM_MAX * DIR_ROW_BYTES


def read_le_u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def map_script_blob_plausible(data: bytes, blob_off: int, blob_len: int) -> None:
    if blob_len == 0:
        raise ValueError("empty blob")
    if blob_off + blob_len > len(data):
        raise ValueError("blob out of file range")
    pos = 0
    while pos < blob_len:
        tag = data[blob_off + pos]
        if tag == 0:
            return
        if pos + 5 > blob_len:
            raise ValueError("truncated 5-byte record before terminator")
        pos += 5
    raise ValueError("no zero tag terminator in blob")


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

    for g in range(MAP_GROUPS_COUNT):
        maxn = MAPNUM_MAX
        if map_groups and g < len(map_groups["group_order"]):
            maxn = len(map_groups[map_groups["group_order"][g]])
        for n in range(MAPNUM_MAX):
            idx = (g * MAPNUM_MAX + n) * DIR_ROW_BYTES
            off = read_le_u32(data, idx)
            ln = read_le_u32(data, idx + 4)
            if n >= maxn:
                if off != 0 or ln != 0:
                    raise SystemExit(f"non-zero unused slot group={g} num={n}")
                continue
            if off == 0 and ln == 0:
                continue
            if off == 0 or ln == 0:
                raise SystemExit(f"invalid row group={g} num={n} off={off} len={ln}")
            if off < GRID_BYTES or off >= len(data):
                raise SystemExit(f"blob_off out of range group={g} num={n}")
            try:
                map_script_blob_plausible(data, off, ln)
            except ValueError as e:
                raise SystemExit(f"group={g} num={n}: {e}") from e

    print("OK", args.pack, f"({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
