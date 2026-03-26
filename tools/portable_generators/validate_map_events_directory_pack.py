#!/usr/bin/env python3
"""
Structural validation of a map-events directory image (grid + optional pool in the same file).

Checks:
  - grid size MAP_GROUPS_COUNT * 256 * 8
  - unused slots zero
  - each non-0/0 blob: magic, version, length equation, row sub-sizes
  - script u32 words: 0, or 0x08…… ROM band, or portable token bands 0x80–0x86

Does not prove resolver closure (needs a loaded ROM + game build).

Usage:
  python validate_map_events_directory_pack.py <pack.bin> [--map-groups path/to/map_groups.json]
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

MAGIC = 0x5645504D
VERSION = 1
HEADER = 16
OBJ_WIRE = 24
WARP_WIRE = 8
COORD_WIRE = 16
BG_WIRE = 12


def read_le_u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def read_le_u16(b: bytes, off: int) -> int:
    return struct.unpack_from("<H", b, off)[0]


def script_word_ok(w: int) -> bool:
    if w == 0:
        return True
    if 0x08000000 <= w < 0x0A000000:
        return True
    if 0x80000000 <= w < 0x86000000:
        return True
    return False


def validate_blob(data: bytes, blob_off: int, blob_len: int) -> None:
    if blob_off + blob_len > len(data):
        raise ValueError("blob out of range")
    if blob_len < HEADER:
        raise ValueError("blob too short for header")
    if read_le_u32(data, blob_off) != MAGIC:
        raise ValueError("bad magic")
    if read_le_u32(data, blob_off + 4) != VERSION:
        raise ValueError("bad version")
    packed = read_le_u32(data, blob_off + 8)
    if read_le_u32(data, blob_off + 12) != 0:
        raise ValueError("reserved field non-zero")
    obj = packed & 0xFF
    warp = (packed >> 8) & 0xFF
    coord = (packed >> 16) & 0xFF
    bg = (packed >> 24) & 0xFF
    if obj > 64 or warp > 64 or coord > 64 or bg > 64:
        raise ValueError("count overflow")
    expect = HEADER + obj * OBJ_WIRE + warp * WARP_WIRE + coord * COORD_WIRE + bg * BG_WIRE
    if expect != blob_len:
        raise ValueError(f"blob_len mismatch want {expect} got {blob_len}")
    pos = blob_off + HEADER
    for _ in range(obj):
        scr = read_le_u32(data, pos + 16)
        if not script_word_ok(scr):
            raise ValueError(f"bad object script word 0x{scr:08X}")
        pos += OBJ_WIRE
    pos += warp * WARP_WIRE
    for _ in range(coord):
        scr = read_le_u32(data, pos + 12)
        if read_le_u16(data, pos + 10) != 0:
            raise ValueError("coord pad non-zero")
        if not script_word_ok(scr):
            raise ValueError(f"bad coord script word 0x{scr:08X}")
        pos += COORD_WIRE
    for _ in range(bg):
        kind = data[pos + 5]
        pad = read_le_u16(data, pos + 6)
        payload = read_le_u32(data, pos + 8)
        if pad != 0:
            raise ValueError("bg pad non-zero")
        if kind in (5, 6, 7, 8):
            continue
        if not script_word_ok(payload):
            raise ValueError(f"bad bg script payload 0x{payload:08X}")
        pos += BG_WIRE


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
            validate_blob(data, off, ln)

    print("OK", args.pack, f"({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ValueError as e:
        print("validate:", e, file=sys.stderr)
        raise SystemExit(1) from e
