#!/usr/bin/env python3
"""
Offline Phase 3 smoke: **`MAP_GRID_BLOCK_WORD_PHASE3_U32_*`** in
**`include/constants/map_grid_block_word.h`** matches **`map_visual_policy_constants.py`**.

Runtime / vanilla `map.json` still use **u16** PRET wire V1 + Phase 2 RAM; Phase 3 is the **documented
u32 on-disk** contract for hacks + **`validate_map_layout_block_bin.py --cell-width 4`**.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
CONSTANTS_H = REPO_ROOT / "include" / "constants" / "map_grid_block_word.h"


def _strip_c_comment(line: str) -> str:
    if "//" in line:
        line = line[: line.index("//")]
    return line.strip()


def _parse_define_int(path: Path, name: str) -> int:
    text = path.read_text(encoding="utf-8")
    for raw in text.splitlines():
        line = _strip_c_comment(raw)
        m = re.match(rf"^#define\s+{re.escape(name)}\s+(.+)$", line)
        if not m:
            continue
        val = m.group(1).strip().rstrip("uUlL")
        return int(val, 0)
    raise KeyError(f"#define {name} not found in {path}")


def main() -> int:
    if not CONSTANTS_H.is_file():
        print(f"check_project_c_phase3_offline: missing {CONSTANTS_H}", file=sys.stderr)
        return 2

    pgen = REPO_ROOT / "tools" / "portable_generators"
    sys.path.insert(0, str(pgen))
    import map_visual_policy_constants as pvc

    pairs = [
        ("MAP_GRID_BLOCK_WORD_LAYOUT_PHASE3_U32", "MAP_GRID_BLOCK_WORD_LAYOUT_PHASE3_U32"),
        ("MAP_GRID_BLOCK_WORD_PHASE3_U32_METATILE_ID_MASK", "MAP_GRID_BLOCK_WORD_PHASE3_U32_METATILE_ID_MASK"),
        ("MAP_GRID_BLOCK_WORD_PHASE3_U32_COLLISION_MASK", "MAP_GRID_BLOCK_WORD_PHASE3_U32_COLLISION_MASK"),
        ("MAP_GRID_BLOCK_WORD_PHASE3_U32_ELEVATION_MASK", "MAP_GRID_BLOCK_WORD_PHASE3_U32_ELEVATION_MASK"),
        ("MAP_GRID_BLOCK_WORD_PHASE3_U32_COLLISION_SHIFT", "MAP_GRID_BLOCK_WORD_PHASE3_U32_COLLISION_SHIFT"),
        ("MAP_GRID_BLOCK_WORD_PHASE3_U32_ELEVATION_SHIFT", "MAP_GRID_BLOCK_WORD_PHASE3_U32_ELEVATION_SHIFT"),
        ("MAP_GRID_BLOCK_WORD_PHASE3_U32_ENCODED_METATILE_COUNT", "MAP_GRID_BLOCK_WORD_PHASE3_U32_ENCODED_METATILE_COUNT"),
    ]
    errors: list[str] = []
    for c_name, py_attr in pairs:
        c_val = _parse_define_int(CONSTANTS_H, c_name)
        py_val = getattr(pvc, py_attr)
        if c_val != py_val:
            errors.append(f"{c_name} C={c_val} != Python {py_attr}={py_val}")

    mid = _parse_define_int(CONSTANTS_H, "MAP_GRID_BLOCK_WORD_PHASE3_U32_METATILE_ID_MASK")
    enc = _parse_define_int(CONSTANTS_H, "MAP_GRID_BLOCK_WORD_PHASE3_U32_ENCODED_METATILE_COUNT")
    if mid + 1 != enc:
        errors.append(f"Phase3 U32 metatile mask+1 != encoded count: {mid + 1} != {enc}")

    if errors:
        for e in errors:
            print(f"check_project_c_phase3_offline: {e}", file=sys.stderr)
        return 1

    print(f"OK: Project C Phase 3 offline checks (u32 layout id {pvc.MAP_GRID_BLOCK_WORD_LAYOUT_PHASE3_U32}, encoded {enc}).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
