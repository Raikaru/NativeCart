"""
Resolve C preprocessor macros to integer values using `gcc -E -dM`.

Used by ROM pack emitters that need VAR_*, LOCALID_*, OBJ_EVENT_GFX_*, etc.
"""

from __future__ import annotations

import re
import subprocess
import tempfile
from pathlib import Path


def load_macros_from_headers(root: Path, includes: list[str]) -> dict[str, int]:
    """
    includes: e.g. ['constants/vars.h', 'constants/event_objects.h']
    Returns mapping NAME -> int for #define NAME <integer-like>.
    """
    src = '#include "global.h"\n' + ''.join(f'#include "{h}"\n' for h in includes)
    with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False, encoding="utf-8") as f:
        f.write(src)
        tmp = f.name
    try:
        cmd = [
            "gcc",
            "-E",
            "-dM",
            "-std=gnu99",
            "-DPORTABLE",
            "-DFIRERED",
            "-I",
            str(root / "include"),
            "-I",
            str(root / "include" / "gba"),
            "-I",
            str(root / "pokefirered_core" / "include"),
            tmp,
        ]
        out = subprocess.check_output(cmd, text=True, stderr=subprocess.PIPE)
    finally:
        Path(tmp).unlink(missing_ok=True)

    defines: dict[str, int] = {}
    for line in out.splitlines():
        m = re.match(r'^#define (\w+)\s+(.+)$', line.strip())
        if not m:
            continue
        name, body = m.group(1), m.group(2).strip()
        # Strip casts like (0x4050) or (TEMP_VARS_START + 0x0) already expanded by -dM
        if body.startswith("(") and body.endswith(")"):
            body = body[1:-1].strip()
        try:
            defines[name] = int(body, 0)
        except ValueError:
            continue
    return defines
