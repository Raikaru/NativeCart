#!/usr/bin/env python3
"""
Offline Direction D smoke (no ROM): **`gFireredPortableCompiledTilesetTable`** parses from generated
**`map_data_portable.c`**, **`headers.h`** **`isSecondary`** map loads, and per-index role caps build.

Run after **`make check-validators`** or in CI beside layout / field-map Python checks.
"""

from __future__ import annotations

import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    pgen = REPO_ROOT / "tools" / "portable_generators"
    sys.path.insert(0, str(pgen))

    from compiled_tileset_table_roles import (
        build_role_tile_caps_for_enforcement,
        default_map_data_portable_path,
        parse_compiled_tileset_table_order,
    )

    table_c = default_map_data_portable_path(REPO_ROOT)
    if not table_c.is_file():
        print(
            f"check_direction_d_offline: missing {table_c} (generate portable map data first)",
            file=sys.stderr,
        )
        return 2

    text = table_c.read_text(encoding="utf-8")
    syms = parse_compiled_tileset_table_order(text)
    n = len(syms)
    build_role_tile_caps_for_enforcement(count=n, repo_root=REPO_ROOT)
    print(f"OK: Direction D offline checks ({n} compiled tileset table slots, role caps derivable).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
