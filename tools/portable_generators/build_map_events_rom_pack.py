#!/usr/bin/env python3
"""
Emit a FIRERED_ROM_MAP_EVENTS_DIRECTORY_PACK_OFF image:

  [ directory grid: MAP_GROUPS_COUNT * 256 * 8 bytes ]
  [ blob pool: per-map blobs referenced by LE u32 offset + length ]

Each directory row: LE u32 blob_off, LE u32 blob_len (0/0 = use compiled map_data_portable).

Per-map blob layout matches cores/firered/portable/firered_portable_rom_map_events_directory.c.

Usage:
  python build_map_events_rom_pack.py <pokefirered_root> <output.bin> [--mode sparse|--mode fill-all]

  --mode sparse (default): all directory rows 0/0 (empty pack skeleton for hand-authoring).
  --mode fill-all:       build blobs from data/maps/**/map.json (vanilla parity / round-trip).

Requires: gcc on PATH (same as load_c_macros_via_gcc) for macro resolution.
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

_PKG = str(Path(__file__).resolve().parent)
if _PKG not in sys.path:
    sys.path.insert(0, _PKG)

from eval_c_macro_int import batch_eval_macro_ints
from load_c_macros_via_gcc import load_macros_from_headers

MACRO_INCLUDES = [
    "constants/vars.h",
    "constants/event_objects.h",
    "constants/map_event_ids.h",
    "constants/event_bg.h",
    "constants/flags.h",
    "constants/weather.h",
    "constants/event_object_movement.h",
    "constants/trainer_types.h",
    "constants/items.h",
    "constants/maps.h",
]

MAP_GROUPS_COUNT = 43
MAPNUM_MAX = 256
DIR_ROW_BYTES = 8
GRID_BYTES = MAP_GROUPS_COUNT * MAPNUM_MAX * DIR_ROW_BYTES

MAGIC = 0x5645504D  # 'MPEV' LE
VERSION = 1

OBJ_MAX = 64
WARP_MAX = 64
COORD_MAX = 64
BG_MAX = 64

OBJ_WIRE = 24
WARP_WIRE = 8
COORD_WIRE = 16
BG_WIRE = 12
HEADER = 16

TOKEN_EVENT = 0x81000000
TOKEN_MAP_OBJECT = 0x85000000


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def collect_macro_names_from_maps(maps: dict) -> set[str]:
    """C identifiers appearing in map.json fields that need numeric expansion."""
    names: set[str] = set()

    def add_str(v):
        if isinstance(v, str) and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", v):
            names.add(v)

    for m in maps.values():
        for e in m.get("object_events", []):
            for key in (
                "local_id",
                "graphics_id",
                "elevation",
                "movement_type",
                "movement_range_x",
                "movement_range_y",
                "trainer_type",
                "trainer_sight_or_berry_tree_id",
                "flag",
                "target_local_id",
            ):
                if key in e:
                    add_str(e[key])
        for e in m.get("warp_events", []):
            if "dest_warp_id" in e:
                add_str(e["dest_warp_id"])
        for e in m.get("coord_events", []):
            if "weather" in e:
                add_str(e["weather"])
            if "var" in e:
                add_str(e["var"])
            if "var_value" in e:
                add_str(e["var_value"])
        for e in m.get("bg_events", []):
            for key in ("item", "flag", "quantity", "player_facing_dir", "secret_base_id"):
                if key in e:
                    add_str(e[key])
    return names


def load_maps(root: Path) -> dict:
    maps = {}
    for path in sorted((root / "data" / "maps").glob("*/map.json")):
        data = read_json(path)
        maps[data["name"]] = data
    return maps


def build_map_indices(map_groups: dict, maps: dict) -> dict:
    indices = {}
    for group_index, group_name in enumerate(map_groups["group_order"]):
        for map_index, map_name in enumerate(map_groups[group_name]):
            if map_name not in maps:
                raise ValueError(f"map_groups references unknown map name: {map_name}")
            mid = maps[map_name]["id"]
            indices[mid] = (group_index, map_index)
    return indices


def parse_map_object_script_table(path: Path) -> dict[str, int]:
    """Symbol -> index in gFireredPortableMapObjectEventScriptPtrs (for 0x85…… tokens)."""
    lines = path.read_text(encoding="utf-8").splitlines()
    start = None
    for i, line in enumerate(lines):
        if "gFireredPortableMapObjectEventScriptPtrs[] = {" in line:
            start = i + 1
            break
    if start is None:
        raise ValueError(f"{path}: map object script table not found")
    out: dict[str, int] = {}
    idx = 0
    for line in lines[start:]:
        if line.strip() == "};":
            break
        m = re.match(r"\s*(\w+)\s*,?\s*$", line)
        if not m:
            raise ValueError(f"{path}: bad map object ptr line: {line!r}")
        out[m.group(1)] = idx
        idx += 1
    return out


def parse_event_script_ptr_table(path: Path) -> dict[str, int]:
    """Symbol -> index in gFireredPortableEventScriptPtrs (for 0x81…… tokens)."""
    lines = path.read_text(encoding="utf-8").splitlines()
    start = None
    for i, line in enumerate(lines):
        if "gFireredPortableEventScriptPtrs[] = {" in line:
            start = i + 1
            break
    if start is None:
        raise ValueError(f"{path}: event script ptr table not found")
    out: dict[str, int] = {}
    idx = 0
    sym_pat = re.compile(r"\(uintptr_t\)\s*&?([A-Za-z_][A-Za-z0-9_]*)")
    for line in lines[start:]:
        if line.strip() == "};":
            break
        m = sym_pat.search(line)
        if not m:
            raise ValueError(f"{path}: bad event ptr line: {line!r}")
        out[m.group(1)] = idx
        idx += 1
    return out


def sym_int(x, macros: dict[str, int]) -> int:
    if isinstance(x, bool):
        return int(x)
    if isinstance(x, int):
        return x
    s = str(x).strip()
    if s in ("", "0", "NULL", "0x0", "FALSE", "TRUE"):
        if s == "TRUE":
            return 1
        return 0
    if re.fullmatch(r"-?[0-9]+", s):
        return int(s, 10)
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 0)
    if s not in macros:
        raise ValueError(f"unknown C symbol {s!r} (add header to load_c_macros list?)")
    return macros[s]


def script_u32(
    name: str | None,
    map_obj: dict[str, int],
    event_scripts: dict[str, int],
) -> int:
    if name in (None, "", "NULL", "0x0"):
        return 0
    if name in map_obj:
        return TOKEN_MAP_OBJECT + map_obj[name]
    if name in event_scripts:
        return TOKEN_EVENT + event_scripts[name]
    raise ValueError(
        f"script symbol {name!r} not in map-object table or gFireredPortableEventScriptPtrs — regenerate portable data?"
    )


def pack_object_wire(
    event: dict,
    map_indices: dict,
    macros: dict[str, int],
    map_obj: dict[str, int],
    event_scripts: dict[str, int],
) -> bytes:
    local_id = sym_int(event.get("local_id", 0), macros)
    gfx = sym_int(event["graphics_id"], macros)
    x = int(event["x"])
    y = int(event["y"])
    if event["type"] == "clone":
        require(event, ("target_local_id", "target_map"))
        kind = macros["OBJ_KIND_CLONE"]
        mg, mn = warp_dest_group_num(event["target_map"], map_indices)
        tid = sym_int(event["target_local_id"], macros)
        union = struct.pack("<BxxxHH", tid, mn, mg)
        scr = 0
        flag = 0
    else:
        kind = macros["OBJ_KIND_NORMAL"]
        require(event, ("elevation", "movement_type", "movement_range_x", "movement_range_y", "trainer_type", "trainer_sight_or_berry_tree_id", "script", "flag"))
        el = sym_int(event["elevation"], macros)
        mv = sym_int(event["movement_type"], macros)
        rx = sym_int(event["movement_range_x"], macros)
        ry = sym_int(event["movement_range_y"], macros)
        tt = sym_int(event["trainer_type"], macros)
        tr = sym_int(event["trainer_sight_or_berry_tree_id"], macros)
        ranges = (ry << 4) | (rx & 0xF)
        union = struct.pack("<BBHHH", el, mv, ranges, tt, tr)
        scr = script_u32(event["script"], map_obj, event_scripts)
        flag = sym_int(event["flag"], macros)
    head = struct.pack("<BBBxhh", local_id, gfx, kind, x, y)
    tail = struct.pack("<I H xx", scr, flag)
    wire = head + union + tail
    if len(wire) != OBJ_WIRE:
        raise RuntimeError("internal object wire size")
    return wire


def warp_dest_group_num(dest_map: str, map_indices: dict) -> tuple[int, int]:
    if dest_map in map_indices:
        return map_indices[dest_map]
    # constants/maps.h — not always integer-expanded by gcc -E -dM
    if dest_map == "MAP_DYNAMIC":
        return 0x7F, 0x7F
    if dest_map == "MAP_UNDEFINED":
        return 0xFF, 0xFF
    raise ValueError(f"warp dest_map {dest_map!r} not in map_indices")


def pack_warp_wire(event: dict, map_indices: dict, macros: dict[str, int]) -> bytes:
    require(event, ("x", "y", "elevation", "dest_warp_id", "dest_map"))
    mg, mn = warp_dest_group_num(event["dest_map"], map_indices)
    return struct.pack(
        "<hhBBBB",
        int(event["x"]),
        int(event["y"]),
        int(event["elevation"]),
        sym_int(event["dest_warp_id"], macros),
        mn,
        mg,
    )


def pack_coord_wire(event: dict, macros: dict[str, int], event_scripts: dict[str, int]) -> bytes:
    require(event, ("type", "x", "y", "elevation"))
    x = int(event["x"])
    y = int(event["y"])
    el = sym_int(event["elevation"], macros)
    if event["type"] == "weather":
        require(event, ("weather",))
        trig = sym_int(event["weather"], macros)
        idx = 0
        scr = 0
    else:
        require(event, ("var", "var_value", "script"))
        trig = sym_int(event["var"], macros)
        idx = sym_int(event["var_value"], macros)
        scr = script_u32(event["script"], {}, event_scripts)
    return struct.pack("<HHBBHH2xI", x, y, el, 0, trig, idx, scr)


def pack_bg_wire(event: dict, macros: dict[str, int], event_scripts: dict[str, int]) -> bytes:
    require(event, ("type", "x", "y", "elevation"))
    x = int(event["x"])
    y = int(event["y"])
    el = sym_int(event["elevation"], macros)
    if event["type"] == "hidden_item":
        require(event, ("item", "flag", "quantity", "underfoot"))
        kind = macros["BG_EVENT_HIDDEN_ITEM"]
        item = sym_int(event["item"], macros)
        flag = sym_int(event["flag"], macros)
        qty = sym_int(event["quantity"], macros)
        hid_start = macros["FLAG_HIDDEN_ITEMS_START"]
        under = 1 if event["underfoot"] else 0
        payload = (
            (item & 0xFFFF)
            | (((flag - hid_start) & 0xFF) << 16)
            | ((qty & 0x7F) << 24)
            | ((under & 1) << 31)
        ) & 0xFFFFFFFF
    elif event["type"] == "secret_base":
        require(event, ("secret_base_id",))
        kind = macros["BG_EVENT_SECRET_BASE"]
        payload = sym_int(event["secret_base_id"], macros) & 0xFFFFFFFF
    else:
        require(event, ("player_facing_dir", "script"))
        kind = sym_int(event["player_facing_dir"], macros)
        payload = script_u32(event["script"], {}, event_scripts)
    return struct.pack("<HHBBxxI", x, y, el, kind, payload)


def require(d: dict, fields: tuple):
    for f in fields:
        if f not in d:
            raise ValueError(f"missing field {f!r} in {d!r}")


def build_blob_for_map(
    map_data: dict,
    map_indices: dict,
    macros: dict[str, int],
    map_obj: dict[str, int],
    event_scripts: dict[str, int],
) -> bytes:
    objs = map_data.get("object_events", [])
    warps = map_data.get("warp_events", [])
    coords = map_data.get("coord_events", [])
    bgs = map_data.get("bg_events", [])
    if len(objs) > OBJ_MAX or len(warps) > WARP_MAX or len(coords) > COORD_MAX or len(bgs) > BG_MAX:
        raise ValueError(f"{map_data.get('name')}: event count exceeds ROM caps")

    parts = []
    for e in objs:
        parts.append(pack_object_wire(e, map_indices, macros, map_obj, event_scripts))
    for e in warps:
        parts.append(pack_warp_wire(e, map_indices, macros))
    for e in coords:
        parts.append(pack_coord_wire(e, macros, event_scripts))
    for e in bgs:
        parts.append(pack_bg_wire(e, macros, event_scripts))

    packed_counts = (len(bgs) << 24) | (len(coords) << 16) | (len(warps) << 8) | len(objs)
    header = struct.pack("<IIII", MAGIC, VERSION, packed_counts, 0)
    blob = header + b"".join(parts)
    expect = HEADER + len(objs) * OBJ_WIRE + len(warps) * WARP_WIRE + len(coords) * COORD_WIRE + len(bgs) * BG_WIRE
    if len(blob) != expect:
        raise RuntimeError("blob length mismatch")
    return blob


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("root", type=Path, help="pokefirered repo root")
    ap.add_argument("out", type=Path, help="output .bin")
    ap.add_argument(
        "--mode",
        choices=("sparse", "fill-all"),
        default="sparse",
        help="sparse: all 0/0 rows; fill-all: emit blobs from map.json",
    )
    args = ap.parse_args()
    root: Path = args.root.resolve()
    out: Path = args.out.resolve()

    grid = bytearray(GRID_BYTES)

    if args.mode == "sparse":
        out.write_bytes(grid)
        print(f"Wrote sparse grid-only pack ({len(grid)} bytes) -> {out}")
        return 0

    macros = load_macros_from_headers(root, MACRO_INCLUDES)
    map_groups = read_json(root / "data" / "maps" / "map_groups.json")
    maps = load_maps(root)
    need = collect_macro_names_from_maps(maps)
    missing = sorted(n for n in need if n not in macros)
    if missing:
        macros.update(batch_eval_macro_ints(root, set(missing), MACRO_INCLUDES))
    map_indices = build_map_indices(map_groups, maps)
    map_obj_path = root / "pokefirered_core" / "generated" / "src" / "data" / "map_object_event_scripts_portable_data.c"
    evt_path = root / "pokefirered_core" / "generated" / "src" / "data" / "event_scripts_portable_data.c"
    map_obj = parse_map_object_script_table(map_obj_path)
    event_scripts = parse_event_script_ptr_table(evt_path)

    pool = bytearray()
    for g, group_name in enumerate(map_groups["group_order"]):
        group_list = map_groups[group_name]
        for n in range(MAPNUM_MAX):
            idx = (g * MAPNUM_MAX + n) * DIR_ROW_BYTES
            if n >= len(group_list):
                continue
            name = group_list[n]
            blob = build_blob_for_map(maps[name], map_indices, macros, map_obj, event_scripts)
            off = GRID_BYTES + len(pool)
            pool.extend(blob)
            struct.pack_into("<II", grid, idx, off, len(blob))

    data = bytes(grid) + bytes(pool)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(data)
    print(f"Wrote map events pack {len(data)} bytes (grid={GRID_BYTES}, pool={len(pool)}) -> {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
