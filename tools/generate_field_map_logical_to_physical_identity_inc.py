#!/usr/bin/env python3
"""Emit src/data/field_map_logical_to_physical_identity.inc (identity 0..1023).

Run from repo root after changing MAP_BG_FIELD_TILE_CHAR_CAPACITY / NUM_TILES_TOTAL.
Post-run validation uses `--strict-identity` (implies injective). For custom remap tables in CI, prefer
`validate_field_map_logical_to_physical_inc.py --strict-injective` (and keep the door contract unless
coordinated with field_door).
"""
from __future__ import annotations

import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "data" / "field_map_logical_to_physical_identity.inc"
COUNT = 1024
VALIDATOR = ROOT / "tools" / "validate_field_map_logical_to_physical_inc.py"


def main() -> int:
    if len(sys.argv) > 1:
        print("usage: generate_field_map_logical_to_physical_identity_inc.py", file=sys.stderr)
        return 2
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(",\n".join(str(i) for i in range(COUNT)) + "\n", encoding="utf-8", newline="\n")
    print("wrote", OUT)
    subprocess.check_call([sys.executable, str(VALIDATOR), str(OUT), "--strict-identity"], cwd=str(ROOT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
