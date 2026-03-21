#!/usr/bin/env python3
"""Wire portable_generated/*_portable_nullfix.h into src_transformed/*.c and drop matching NULL macros."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRANS = ROOT / "src_transformed"
GEN = TRANS / "portable_generated"

SYM_RE = re.compile(r"^#define\s+(\w+)\s+\1_Portable\s*$")
NULL_LINE_RE = re.compile(
    r"^#define\s+(\w+)\s+\(\(const\s+u(?:8|16|32)\s*\*\)NULL\)\s*$"
)


def symbols_in_header(hpath: Path) -> list[str]:
    syms = []
    for line in hpath.read_text(encoding="utf-8", errors="replace").splitlines():
        m = SYM_RE.match(line.strip())
        if m:
            syms.append(m.group(1))
    return syms


def insert_include(text: str, stem: str) -> str:
    inc = f'#include "portable_generated/{stem}_portable_nullfix.h"'
    if inc in text:
        return text

    lines = text.splitlines(keepends=True)
    # Only the file's initial include block (before first non-# non-blank line).
    # Never insert into a later #ifdef PORTABLE inside a function.
    last_include = -1
    for k, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("#include"):
            last_include = k
            continue
        if last_include < 0:
            continue
        if stripped == "" or stripped.startswith("//"):
            continue
        if stripped.startswith("#"):
            # allow #define / #pragma / #if at top (unlikely between includes)
            if k < 80:
                continue
        break

    if last_include < 0:
        raise RuntimeError("no includes")

    insert_line = f"\n#ifdef PORTABLE\n{inc}\n#endif\n"
    lines.insert(last_include + 1, insert_line)
    return "".join(lines)


def strip_null_defines(text: str, symbols: set[str]) -> str:
    out = []
    for line in text.splitlines(keepends=True):
        m = NULL_LINE_RE.match(line.strip())
        if m and m.group(1) in symbols:
            continue
        out.append(line)
    return "".join(out)


def main() -> None:
    for hpath in sorted(GEN.glob("*_portable_nullfix.h")):
        stem = hpath.name.replace("_portable_nullfix.h", "")
        cpath = TRANS / f"{stem}.c"
        if not cpath.is_file():
            continue
        syms = symbols_in_header(hpath)
        if not syms:
            print("skip empty", hpath.name)
            continue
        text = cpath.read_text(encoding="utf-8", errors="replace")
        text = insert_include(text, stem)
        text = strip_null_defines(text, set(syms))
        cpath.write_text(text, encoding="utf-8")
        print("patched", cpath.name, len(syms), "symbols")


if __name__ == "__main__":
    main()
