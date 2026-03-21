#!/usr/bin/env python3
"""Rebuild union_room_chat_display_portable_nullfix.h from src/union_room_chat_display.c INCBINs."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INCBIN_RE = re.compile(
    r'static\s+const\s+(u8|u16|u32)\s+(\w+)\s*\[\s*\]\s*=\s*INCBIN_(U8|U16|U32)\s*\(\s*"([^"]+)"\s*\)'
)


def emit_array(ctype: str, sym: str, kind: str, gpath: str) -> str:
    bin_path = ROOT / gpath
    data = bin_path.read_bytes()
    lines_out: list[str] = [f"/* {gpath} */", f"static const {ctype} {sym}_Portable[] = {{"]

    def flush_row(row: list[str]) -> None:
        if row:
            lines_out.append("    " + ", ".join(row) + ",")

    if kind == "U8":
        row = []
        for b in data:
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
    else:
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
    return "\n".join(lines_out)


def main() -> None:
    text = (ROOT / "src/union_room_chat_display.c").read_text(encoding="utf-8")
    order: list[tuple[str, str, str, str]] = []
    for m in INCBIN_RE.finditer(text):
        order.append((m.group(2), m.group(1), m.group(3), m.group(4)))

    chunks = [
        "/* auto-generated — tools/regen_union_room_chat_display_nullfix.py */",
        "#ifdef PORTABLE",
        '#include "global.h"',
        "",
    ]
    for sym, ctype, kind, gpath in order:
        chunks.append(emit_array(ctype, sym, kind, gpath))
        chunks.append("")
    chunks.append("#endif /* PORTABLE */")

    out = ROOT / "src_transformed/portable_generated/union_room_chat_display_portable_nullfix.h"
    out.write_text("\n".join(chunks), encoding="utf-8")
    print(out, out.stat().st_size)


if __name__ == "__main__":
    main()
