#!/usr/bin/env python3
"""
Emit a dense FIRERED_ROM_MAP_CONNECTIONS_PACK_OFF blob matching
cores/firered/portable/firered_portable_rom_map_connections.c.

Grid: MAP_GROUPS_COUNT x 256 rows of 100 bytes each (vanilla MAP_GROUPS_COUNT from constants).
Unused (group, mapNum) slots must be zero-filled.

Usage:
  python build_map_connections_rom_pack.py <pokefirered_root> <output.bin>

Requires the same data/maps JSON layout as generate_portable_map_data.py.
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

# Must match include/constants/map_groups.h MAP_GROUPS_COUNT for the target build.
MAP_GROUPS_COUNT = 43
MAPNUM_MAX = 256
MAX_CONN = 8
ROW_BYTES = 4 + MAX_CONN * 12

DIRECTION_MAP = {
    'up': 2,  # CONNECTION_NORTH
    'down': 1,  # CONNECTION_SOUTH
    'left': 3,  # CONNECTION_WEST
    'right': 4,  # CONNECTION_EAST
    'dive': 5,
    'emerge': 6,
}


def read_json(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def load_maps(root: Path) -> dict:
    maps = {}
    for path in sorted((root / 'data' / 'maps').glob('*/map.json')):
        data = read_json(path)
        maps[data['name']] = data
    return maps


def build_map_indices(map_groups: dict, maps: dict) -> dict:
    indices = {}
    for group_index, group_name in enumerate(map_groups['group_order']):
        for map_index, map_name in enumerate(map_groups[group_name]):
            if map_name not in maps:
                raise ValueError(f'map_groups references unknown map name: {map_name}')
            indices[map_name] = (group_index, map_index)
    return indices


def build_map_id_to_group_num(map_groups: dict, maps: dict, map_indices: dict) -> dict:
    """map.json `id` (e.g. MAP_ROUTE1) -> (group, num) in gMapGroups."""
    out = {}
    for map_name in map_indices:
        mid = maps[map_name].get('id')
        if not mid:
            raise ValueError(f'{map_name}: missing id in map.json')
        out[mid] = map_indices[map_name]
    return out


def pack_row(
    connections: list | None,
    map_id_to_group_num: dict,
    map_name: str,
) -> bytes:
    if not connections:
        return b'\x00' * ROW_BYTES
    if len(connections) > MAX_CONN:
        raise ValueError(f'{map_name}: too many connections ({len(connections)} > {MAX_CONN})')
    out = bytearray(ROW_BYTES)
    struct.pack_into('<I', out, 0, len(connections))
    for i, c in enumerate(connections):
        direction = DIRECTION_MAP.get(c['direction'])
        if direction is None:
            raise ValueError(f'{map_name}: bad direction {c["direction"]!r}')
        target = c['map']
        if target not in map_id_to_group_num:
            raise ValueError(f'{map_name}: unknown target map id {target!r}')
        mg, mn = map_id_to_group_num[target]
        offset = int(c['offset'])
        struct.pack_into('<B3xIBB2x', out, 4 + i * 12, direction, offset & 0xFFFFFFFF, mg, mn)
    return bytes(out)


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    out_path = Path(sys.argv[2]).resolve()
    map_groups = read_json(root / 'data' / 'maps' / 'map_groups.json')
    maps = load_maps(root)
    map_indices = build_map_indices(map_groups, maps)
    map_id_to_group_num = build_map_id_to_group_num(map_groups, maps, map_indices)

    pack = bytearray(MAP_GROUPS_COUNT * MAPNUM_MAX * ROW_BYTES)

    for g, group_name in enumerate(map_groups['group_order']):
        group_list = map_groups[group_name]
        for n in range(MAPNUM_MAX):
            idx = (g * MAPNUM_MAX + n) * ROW_BYTES
            if n >= len(group_list):
                continue
            map_name = group_list[n]
            row = pack_row(maps[map_name].get('connections'), map_id_to_group_num, map_name)
            pack[idx : idx + ROW_BYTES] = row

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(pack)
    print(f'Wrote {len(pack)} bytes to {out_path}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
