#!/usr/bin/env python3
"""
Find #define SYM ((const *)NULL) lines in src_transformed/*.c that are visible
when PORTABLE is defined (would break SDL builds when code dereferences them).

CPP state machine is intentionally small (ifdef/ifndef/else/endif only).
"""
from __future__ import annotations

import re
from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRANS = ROOT / "src_transformed"
SRC = ROOT / "src"

NULL_DEF_RE = re.compile(
    r'^#define\s+(\w+)\s+\(\(const\s+(u8|u16|u32)\s*\*\)NULL\)'
)
INCBIN_RE = re.compile(
    r'static\s+const\s+(u8|u16|u32)\s+(\w+)\s*\[\s*\]\s*=\s*INCBIN_(U8|U16|U32)\s*\(\s*"([^"]+)"\s*\)\s*;'
)


class S(Enum):
    TOP = auto()  # portable active
    IFDEF_P = auto()  # inside #ifdef PORTABLE
    ELSE_AFTER_IFDEF_P = auto()  # #else branch: !PORTABLE
    IFNDEF_P = auto()  # inside #ifndef PORTABLE
    ELSE_AFTER_IFNDEF_P = auto()  # #else branch: PORTABLE


@dataclass
class Frame:
    state: S
    base: S  # state to restore on endif


def portable_sees_null(stack: list[Frame]) -> bool:
    if not stack:
        return True
    top = stack[-1].state
    if top in (S.IFDEF_P, S.TOP):
        return True
    if top == S.ELSE_AFTER_IFDEF_P:
        return False
    if top == S.IFNDEF_P:
        return False
    if top == S.ELSE_AFTER_IFNDEF_P:
        return True
    return True


def parse_stack(line: str, stack: list[Frame]) -> None:
    s = line.strip()
    if s.startswith("#ifdef PORTABLE"):
        stack.append(Frame(S.IFDEF_P, stack[-1].state if stack else S.TOP))
    elif s.startswith("#ifndef PORTABLE"):
        stack.append(Frame(S.IFNDEF_P, stack[-1].state if stack else S.TOP))
    elif s == "#else":
        if not stack:
            return
        t = stack[-1].state
        if t == S.IFDEF_P:
            stack[-1] = Frame(S.ELSE_AFTER_IFDEF_P, stack[-1].base)
        elif t == S.IFNDEF_P:
            stack[-1] = Frame(S.ELSE_AFTER_IFNDEF_P, stack[-1].base)
    elif s.startswith("#endif"):
        if stack:
            stack.pop()


def scan_file(path: Path) -> list[tuple[int, str, str]]:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    stack: list[Frame] = []
    bad: list[tuple[int, str, str]] = []
    for i, line in enumerate(lines):
        if line.strip().startswith("#"):
            parse_stack(line, stack)
        m = NULL_DEF_RE.match(line.strip())
        if m and portable_sees_null(stack):
            bad.append((i + 1, m.group(1), m.group(2)))
    return bad


def load_incbins(src_path: Path) -> dict[str, tuple[str, str, str]]:
    """symbol -> (ctype, incbin_kind, path)"""
    if not src_path.is_file():
        return {}
    text = src_path.read_text(encoding="utf-8", errors="replace")
    out = {}
    for m in INCBIN_RE.finditer(text):
        ctype, sym, kind, gpath = m.group(1), m.group(2), m.group(3), m.group(4)
        out[sym] = (ctype, kind, gpath)
    return out


def emit_array(ctype: str, sym: str, kind: str, gpath: str) -> str:
    bin_path = ROOT / gpath
    if not bin_path.is_file():
        return f"/* MISSING FILE {gpath} for {sym} */\n"
    data = bin_path.read_bytes()
    lines_out: list[str] = [f"/* {gpath} */", f"static const {ctype} {sym}_Portable[] = {{"]

    def flush_row(row: list[str]) -> None:
        if row:
            lines_out.append("    " + ", ".join(row) + ",")

    if kind == "U8":
        row = []
        for j, b in enumerate(data):
            row.append(f"0x{b:02X}")
            if len(row) >= 12:
                flush_row(row)
                row = []
        flush_row(row)
    elif kind == "U16":
        row = []
        for j in range(0, len(data) & ~1, 2):
            w = data[j] | (data[j + 1] << 8)
            row.append(format(w, "#06x"))
            if len(row) >= 8:
                flush_row(row)
                row = []
        flush_row(row)
    else:  # U32
        row = []
        for j in range(0, len(data) & ~3, 4):
            w = (
                data[j]
                | (data[j + 1] << 8)
                | (data[j + 2] << 16)
                | (data[j + 3] << 24)
            )
            row.append(format(w, "#010x"))
            if len(row) >= 8:
                flush_row(row)
                row = []
        flush_row(row)

    lines_out.append("};")
    lines_out.append(f"#define {sym} {sym}_Portable")
    return "\n".join(lines_out) + "\n"


def main() -> None:
    all_bad: dict[str, list[tuple[int, str, str]]] = {}
    for c in sorted(TRANS.glob("*.c")):
        hits = scan_file(c)
        if hits:
            all_bad[c.name] = hits

    gen_dir = TRANS / "portable_generated"
    gen_dir.mkdir(exist_ok=True)

    summary = []
    for fname, hits in sorted(all_bad.items()):
        # Same symbol can be "#define ... NULL" twice in a file (duplicate stubs).
        seen_syms: set[str] = set()
        deduped: list[tuple[int, str, str]] = []
        for line, sym, ct in hits:
            if sym in seen_syms:
                continue
            seen_syms.add(sym)
            deduped.append((line, sym, ct))
        hits = deduped

        src_path = SRC / fname
        incbins = load_incbins(src_path)
        chunks = [
            f"/* auto-generated for {fname} — do not edit */",
            "#ifdef PORTABLE",
            '#include "global.h"',
            "",
        ]
        missing = []
        for _line, sym, _ct in hits:
            if sym not in incbins:
                missing.append(sym)
                continue
            ctype, kind, gpath = incbins[sym]
            chunks.append(emit_array(ctype, sym, kind, gpath))
            chunks.append("")
        if missing:
            chunks.insert(
                3,
                "/* WARNING: no matching INCBIN in src/ for: "
                + ", ".join(missing)
                + " — left as NULL in .c until fixed */\n",
            )
        chunks.append("#endif /* PORTABLE */")

        out_h = gen_dir / (Path(fname).stem + "_portable_nullfix.h")
        out_h.write_text("\n".join(chunks), encoding="utf-8")
        summary.append(f"{fname}: {len(hits)} null-defs portable-visible, src incbins matched {len(hits)-len(missing)}, missing {len(missing)}")

    (gen_dir / "README.txt").write_text(
        "Headers in this folder are generated by tools/portable_null_asset_audit.py\n"
        "Each corresponds to a .c file that had NULL graphics macros visible under PORTABLE.\n",
        encoding="utf-8",
    )

    print(f"Wrote {len(all_bad)} headers to {gen_dir}")
    for s in summary[:60]:
        print(s)
    if len(summary) > 60:
        print(f"... and {len(summary)-60} more")


if __name__ == "__main__":
    main()
