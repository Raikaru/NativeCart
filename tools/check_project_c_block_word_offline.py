#!/usr/bin/env python3
"""
Offline Project C smoke: **`constants/map_grid_block_word.h`** Phase 2 + PRET wire V1 defines match
**`tools/portable_generators/map_visual_policy_constants.py`**. **`global.fieldmap.h`** must alias
**`MAPGRID_*`** to **`MAP_GRID_BLOCK_WORD_*`** (grep, not numeric parse).

Does not build the ROM — complements **`STATIC_ASSERT`**s in **`fieldmap.c`** for CI/agents
without agbcc.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
CONSTANTS_H = REPO_ROOT / "include" / "constants" / "map_grid_block_word.h"
GLOBAL_FIELDMAP_H = REPO_ROOT / "include" / "global.fieldmap.h"


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
        val = m.group(1).strip()
        val = val.rstrip("uUlL")
        return int(val, 0)
    raise KeyError(f"#define {name} not found in {path}")


def main() -> int:
    if not CONSTANTS_H.is_file():
        print(f"check_project_c_block_word_offline: missing {CONSTANTS_H}", file=sys.stderr)
        return 2
    if not GLOBAL_FIELDMAP_H.is_file():
        print(f"check_project_c_block_word_offline: missing {GLOBAL_FIELDMAP_H}", file=sys.stderr)
        return 2

    gfm = GLOBAL_FIELDMAP_H.read_text(encoding="utf-8")
    for needle in (
        "#define MAPGRID_METATILE_ID_MASK MAP_GRID_BLOCK_WORD_METATILE_ID_MASK",
        "#define MAPGRID_COLLISION_MASK   MAP_GRID_BLOCK_WORD_COLLISION_MASK",
        "#define MAPGRID_ELEVATION_MASK   MAP_GRID_BLOCK_WORD_ELEVATION_MASK",
        "#define MAPGRID_COLLISION_SHIFT  MAP_GRID_BLOCK_WORD_COLLISION_SHIFT",
        "#define MAPGRID_ELEVATION_SHIFT  MAP_GRID_BLOCK_WORD_ELEVATION_SHIFT",
    ):
        if needle not in gfm:
            print(f"check_project_c_block_word_offline: missing in global.fieldmap.h: {needle}", file=sys.stderr)
            return 1

    pgen = REPO_ROOT / "tools" / "portable_generators"
    sys.path.insert(0, str(pgen))
    import map_visual_policy_constants as pvc

    pairs = [
        ("MAP_GRID_BLOCK_WORD_LAYOUT_VANILLA", "MAP_GRID_BLOCK_WORD_LAYOUT_VANILLA"),
        ("MAP_GRID_BLOCK_WORD_LAYOUT_PHASE2_U16", "MAP_GRID_BLOCK_WORD_LAYOUT_PHASE2_U16"),
        ("MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK", "MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK"),
        ("MAP_GRID_BLOCK_WORD_WIRE_V1_COLLISION_MASK", "MAP_GRID_BLOCK_WORD_WIRE_V1_COLLISION_MASK"),
        ("MAP_GRID_BLOCK_WORD_WIRE_V1_ELEVATION_MASK", "MAP_GRID_BLOCK_WORD_WIRE_V1_ELEVATION_MASK"),
        ("MAP_GRID_BLOCK_WORD_WIRE_V1_COLLISION_SHIFT", "MAP_GRID_BLOCK_WORD_WIRE_V1_COLLISION_SHIFT"),
        ("MAP_GRID_BLOCK_WORD_WIRE_V1_ELEVATION_SHIFT", "MAP_GRID_BLOCK_WORD_WIRE_V1_ELEVATION_SHIFT"),
        ("MAP_GRID_BLOCK_WORD_METATILE_ID_MASK", "MAP_GRID_BLOCK_WORD_METATILE_ID_MASK"),
        ("MAP_GRID_BLOCK_WORD_COLLISION_MASK", "MAP_GRID_BLOCK_WORD_COLLISION_MASK"),
        ("MAP_GRID_BLOCK_WORD_ELEVATION_MASK", "MAP_GRID_BLOCK_WORD_ELEVATION_MASK"),
        ("MAP_GRID_BLOCK_WORD_COLLISION_SHIFT", "MAP_GRID_BLOCK_WORD_COLLISION_SHIFT"),
        ("MAP_GRID_BLOCK_WORD_ELEVATION_SHIFT", "MAP_GRID_BLOCK_WORD_ELEVATION_SHIFT"),
        ("MAP_GRID_BLOCK_WORD_ENCODED_METATILE_COUNT", "MAP_GRID_METATILE_ID_SPACE_SIZE"),
    ]

    errors: list[str] = []
    for c_name, py_attr in pairs:
        c_val = _parse_define_int(CONSTANTS_H, c_name)
        py_val = getattr(pvc, py_attr)
        if c_val != py_val:
            errors.append(f"{c_name} C={c_val} != Python {py_attr}={py_val}")

    c_mid = _parse_define_int(CONSTANTS_H, "MAP_GRID_BLOCK_WORD_METATILE_ID_MASK")
    c_enc = _parse_define_int(CONSTANTS_H, "MAP_GRID_BLOCK_WORD_ENCODED_METATILE_COUNT")
    if c_mid + 1 != c_enc:
        errors.append(f"Phase2 metatile mask+1 != encoded count: {c_mid + 1} != {c_enc}")

    w_mid = _parse_define_int(CONSTANTS_H, "MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK")
    if w_mid + 1 != 1024:
        errors.append(f"Wire V1 metatile mask+1 != 1024: {w_mid + 1}")

    if errors:
        for e in errors:
            print(f"check_project_c_block_word_offline: {e}", file=sys.stderr)
        return 1

    print(
        "OK: Project C block-word offline checks "
        f"(Phase2 encoded {c_enc}, wire V1 + Phase2 constants match Python; MAPGRID aliases present)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
