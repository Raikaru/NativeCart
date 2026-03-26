#!/usr/bin/env python3
"""Validate src/data/field_map_logical_to_physical_identity.inc (Direction B table).

Readiness checks for remap-capable builds:

- length == 1024, each entry 10-bit-clean and in [0, 1023]
- optional strict identity (entry[i] == i)
- door-reserved tail identity (unless --no-door-contract) — matches MAP_BG_FIELD_DOOR_TILE_* / field_door.c
- collision detection: multiple logical indices mapping to the same physical TID (warn by default;
  --strict-injective makes that a hard error)

See docs/architecture/project_b_field_tileset_redesign.md (Direction B table contract).
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

_PGEN = pathlib.Path(__file__).resolve().parent / "portable_generators"
if str(_PGEN) not in sys.path:
    sys.path.insert(0, str(_PGEN))
from map_visual_policy_constants import (
    MAP_BG_FIELD_DOOR_TILE_RESERVED_COUNT,
    MAP_BG_FIELD_DOOR_TILE_RESERVED_FIRST,
    MAP_BG_FIELD_TILESET_MAX_TILE_INDEX,
    NUM_TILES_IN_PRIMARY,
    NUM_TILES_TOTAL,
)

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_INC = ROOT / "src" / "data" / "field_map_logical_to_physical_identity.inc"
COUNT = NUM_TILES_TOTAL
DOOR_RESERVED = MAP_BG_FIELD_DOOR_TILE_RESERVED_COUNT
DOOR_FIRST = MAP_BG_FIELD_DOOR_TILE_RESERVED_FIRST
# GBA tilemap halfword: 10-bit tile number (Project B), not map block-word metatile field.
TID_MASK = 0x3FF


def _parse_values(text: str) -> list[int]:
    parts = re.split(r"[,\s]+", text.strip())
    out: list[int] = []
    for p in parts:
        if not p:
            continue
        out.append(int(p, 10))
    return out


def _collisions_by_physical(vals: list[int]) -> dict[int, list[int]]:
    by_phys: dict[int, list[int]] = {}
    for logical, phys in enumerate(vals):
        by_phys.setdefault(phys, []).append(logical)
    return {p: logs for p, logs in by_phys.items() if len(logs) > 1}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "path",
        nargs="?",
        type=pathlib.Path,
        default=DEFAULT_INC,
        help="path to .inc (default: src/data/field_map_logical_to_physical_identity.inc)",
    )
    ap.add_argument(
        "--strict-identity",
        action="store_true",
        help="require entry[i] == i for all i (vanilla / shipped identity table)",
    )
    ap.add_argument(
        "--no-door-contract",
        action="store_true",
        help="skip door scratch check (indices %d..%d must map to self until field_door upload matches)"
        % (DOOR_FIRST, COUNT - 1),
    )
    ap.add_argument(
        "--strict-injective",
        action="store_true",
        help="error if two logical indices map to the same physical TID (no shared physical tiles)",
    )
    ap.add_argument(
        "--no-collision-warnings",
        action="store_true",
        help="do not print collision warnings (strict-injective still errors when set)",
    )
    ap.add_argument(
        "--hint-primary-secondary",
        action="store_true",
        help="print non-fatal notes when a primary logical (0..639) maps to physical >= %d or secondary to < %d"
        % (NUM_TILES_IN_PRIMARY, NUM_TILES_IN_PRIMARY),
    )
    args = ap.parse_args()
    path: pathlib.Path = args.path
    if not path.is_file():
        print("error: missing file", path, file=sys.stderr)
        return 2

    vals = _parse_values(path.read_text(encoding="utf-8"))
    if len(vals) != COUNT:
        print("error: expected %d entries, got %d" % (COUNT, len(vals)), file=sys.stderr)
        return 1

    for i, v in enumerate(vals):
        if v < 0 or v > MAP_BG_FIELD_TILESET_MAX_TILE_INDEX:
            print("error: entry[%d]=%r out of 10-bit range" % (i, v), file=sys.stderr)
            return 1
        if v != (v & TID_MASK):
            print("error: entry[%d]=%r has non-TID high bits" % (i, v), file=sys.stderr)
            return 1

    if args.strict_identity:
        for i, v in enumerate(vals):
            if v != i:
                print("error: strict identity failed at [%d]: %d" % (i, v), file=sys.stderr)
                return 1

    if not args.no_door_contract:
        for i in range(DOOR_FIRST, COUNT):
            if vals[i] != i:
                print(
                    "error: door contract: entry[%d] must be %d (see MAP_BG_FIELD_DOOR_TILE_RESERVED_* / field_door.c)"
                    % (i, i),
                    file=sys.stderr,
                )
                return 1

    collisions = _collisions_by_physical(vals)
    if collisions:
        if args.strict_injective:
            for phys in sorted(collisions.keys()):
                print(
                    "error: non-injective: physical %d <- logicals %s"
                    % (phys, ", ".join(str(x) for x in sorted(collisions[phys]))),
                    file=sys.stderr,
                )
            return 1
        if not args.no_collision_warnings:
            print(
                "warning: %d physical TID(s) are targeted by multiple logical indices (last writer wins on upload)"
                % len(collisions),
                file=sys.stderr,
            )
            for phys in sorted(collisions.keys())[:32]:
                logs = sorted(collisions[phys])
                print("  physical %4d <- logical %s" % (phys, ", ".join(str(x) for x in logs)), file=sys.stderr)
            if len(collisions) > 32:
                print("  ... and %d more physical slot(s)" % (len(collisions) - 32), file=sys.stderr)

    if args.hint_primary_secondary:
        max_notes = 64
        notes = 0
        for log in range(0, NUM_TILES_IN_PRIMARY):
            if notes >= max_notes:
                break
            p = vals[log]
            if p >= NUM_TILES_IN_PRIMARY:
                print(
                    "note: primary-semantic logical %d -> physical %d (valid if intentional)" % (log, p),
                    file=sys.stderr,
                )
                notes += 1
        for log in range(NUM_TILES_IN_PRIMARY, DOOR_FIRST):
            if notes >= max_notes:
                break
            p = vals[log]
            if p < NUM_TILES_IN_PRIMARY:
                print(
                    "note: secondary-semantic logical %d -> physical %d (valid if intentional)" % (log, p),
                    file=sys.stderr,
                )
                notes += 1
        if notes >= max_notes:
            print("note: hint cap reached (%d messages); refine table or drop --hint-primary-secondary" % max_notes, file=sys.stderr)

    print("ok:", path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
