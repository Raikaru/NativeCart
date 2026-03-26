#!/usr/bin/env python3
"""
Offline Direction B smoke (no ROM): **`src/data/field_map_logical_to_physical_identity.inc`**
must exist; then run **`validate_field_map_logical_to_physical_inc.py`** (door tail, range, collisions).

Use in CI / **`make check-direction-b-offline`** beside Direction D and layout validators.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
INC = REPO_ROOT / "src" / "data" / "field_map_logical_to_physical_identity.inc"
VALIDATOR = REPO_ROOT / "tools" / "validate_field_map_logical_to_physical_inc.py"


def main() -> int:
    if not INC.is_file():
        print(
            f"check_direction_b_offline: missing {INC}",
            file=sys.stderr,
        )
        return 2
    r = subprocess.run(
        [sys.executable, str(VALIDATOR)],
        cwd=str(REPO_ROOT),
    )
    if r.returncode != 0:
        return r.returncode
    print("OK: Direction B offline checks (logical->physical .inc present, validator passed).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
