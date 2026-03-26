"""Evaluate C macro names to integers via `gcc -E -P` (for symbols gcc -dM leaves unevaluated)."""

from __future__ import annotations

import re
import subprocess
import tempfile
from pathlib import Path

def _strip_integer_casts(expr: str) -> str:
    e = expr.strip()
    while True:
        m = re.match(r"^\(unsigned long long\)\s*\((.*)\)\s*$", e, re.DOTALL)
        if m:
            e = m.group(1).strip()
            continue
        m = re.match(r"^\(unsigned long long\)\s*(.+)$", e)
        if m:
            e = m.group(1).strip()
            continue
        break
    return e


def _safe_eval_int(expr: str) -> int:
    e = _strip_integer_casts(expr)
    if not re.fullmatch(r"[0-9xXa-fA-FuUlL\+\-\*\(\)\|\&\~\s]+", e):
        raise ValueError(f"unsafe macro expr: {expr!r}")
    return int(eval(e, {"__builtins__": {}}, {}))  # noqa: S307


def batch_eval_macro_ints(root: Path, names: set[str], includes: list[str]) -> dict[str, int]:
    if not names:
        return {}
    lines = ['#include "global.h"']
    for h in includes:
        lines.append(f'#include "{h}"')
    for n in sorted(names):
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", n):
            raise ValueError(f"bad macro name for eval: {n!r}")
        safe = re.sub(r"[^A-Za-z0-9_]", "_", n)
        lines.append(f"static unsigned long long evalsym_{safe} = (unsigned long long)({n});")
    src = "\n".join(lines) + "\n"
    with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False, encoding="utf-8") as f:
        f.write(src)
        tmp = f.name
    try:
        cmd = [
            "gcc",
            "-E",
            "-P",
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

    by_safe: dict[str, int] = {}
    for line in out.splitlines():
        if "evalsym_" not in line or " = " not in line:
            continue
        eq = line.find(" = ")
        lhs = line[:eq].strip()
        rhs = line[eq + 3 :].strip().rstrip(";").strip()
        m = re.search(r"(evalsym_\w+)\s*$", lhs)
        if not m:
            continue
        by_safe[m.group(1)] = _safe_eval_int(rhs)

    out_map: dict[str, int] = {}
    for n in names:
        safe = "evalsym_" + re.sub(r"[^A-Za-z0-9_]", "_", n)
        key = safe
        if key not in by_safe:
            raise RuntimeError(f"gcc -E did not emit assignment for macro {n!r} (expected {key})")
        out_map[n] = by_safe[key]
    return out_map
