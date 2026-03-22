#!/usr/bin/env python3
"""
Read-only inventory: map-related .c files under cores/firered/generated/data.

For each wrapper, prints line count and, when the file is a single #include "…"
to another .c under the repo, prints resolved path stats: total lines, counts of
lines starting with `static const`, `extern const`, and C line comments /
block-comment openers (rough "section" dividers in generated C).

No third-party dependencies (stdlib only). Does not modify any files.

Examples:
  python tools/inventory_map_generated_data.py
  python tools/inventory_map_generated_data.py --json
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


INCLUDE_RE = re.compile(
    r'^\s*#\s*include\s+"(?P<path>[^"]+\.c)"\s*$',
    re.IGNORECASE,
)
STATIC_CONST_RE = re.compile(r"^\s*static\s+const\b")
EXTERN_CONST_RE = re.compile(r"^\s*extern\s+const\b")
# Rough "section" markers: full-line // … or line starting a block comment
LINE_COMMENT_RE = re.compile(r"^\s*//")
BLOCK_OPEN_RE = re.compile(r"^\s*/\*")


def count_lines_stream(path: Path) -> int:
    n = 0
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            n += chunk.count(b"\n")
    return n


def analyze_c_text(path: Path) -> dict[str, int]:
    stats = {
        "lines": 0,
        "static_const_lines": 0,
        "extern_const_lines": 0,
        "line_comment_lines": 0,
        "block_comment_open_lines": 0,
    }
    with path.open("r", encoding="utf-8", errors="replace", newline="") as f:
        for line in f:
            stats["lines"] += 1
            if STATIC_CONST_RE.match(line):
                stats["static_const_lines"] += 1
            if EXTERN_CONST_RE.match(line):
                stats["extern_const_lines"] += 1
            if LINE_COMMENT_RE.match(line):
                stats["line_comment_lines"] += 1
            if BLOCK_OPEN_RE.match(line):
                stats["block_comment_open_lines"] += 1
    return stats


def try_resolve_include(wrapper: Path) -> Path | None:
    text = wrapper.read_text(encoding="utf-8", errors="replace")
    lines = [ln.strip("\r\n") for ln in text.splitlines()]
    non_empty = [ln for ln in lines if ln.strip()]
    if len(non_empty) != 1:
        return None
    m = INCLUDE_RE.match(non_empty[0])
    if not m:
        return None
    rel = m.group("path").replace("\\", "/")
    target = (wrapper.parent / rel).resolve()
    if not target.is_file():
        return None
    return target


def main() -> int:
    repo = Path(__file__).resolve().parent.parent
    p = argparse.ArgumentParser(
        description="Inventory map-related generated data .c under cores/firered/generated/data."
    )
    p.add_argument(
        "--data-dir",
        type=Path,
        default=repo / "cores" / "firered" / "generated" / "data",
        help="Directory to scan (default: cores/firered/generated/data)",
    )
    p.add_argument(
        "--json",
        action="store_true",
        help="Print JSON array instead of text",
    )
    args = p.parse_args()
    data_dir: Path = args.data_dir
    if not data_dir.is_dir():
        print(f"error: not a directory: {data_dir}", file=sys.stderr)
        return 2

    entries: list[dict[str, object]] = []
    for path in sorted(data_dir.glob("*.c")):
        if "map" not in path.name.lower():
            continue
        wrapper_lines = count_lines_stream(path)
        resolved = try_resolve_include(path)
        item: dict[str, object] = {
            "wrapper": str(path.relative_to(repo)).replace("\\", "/"),
            "wrapper_lines": wrapper_lines,
            "resolved": None,
        }
        if resolved is not None and resolved.is_file():
            try:
                rel_resolved = str(resolved.relative_to(repo)).replace("\\", "/")
            except ValueError:
                rel_resolved = str(resolved)
            item["resolved"] = rel_resolved
            item["resolved_stats"] = analyze_c_text(resolved)
        entries.append(item)

    if args.json:
        json.dump(entries, sys.stdout, indent=2)
        sys.stdout.write("\n")
        return 0

    print(f"repo={repo}")
    print(f"scanned={data_dir} map-related .c count={len(entries)}")
    for e in entries:
        print()
        print(f"wrapper: {e['wrapper']}  ({e['wrapper_lines']} lines)")
        if e.get("resolved"):
            st = e["resolved_stats"]
            assert isinstance(st, dict)
            print(f"  -> {e['resolved']}")
            print(
                f"     lines={st['lines']}  "
                f"static_const_lines={st['static_const_lines']}  "
                f"extern_const_lines={st['extern_const_lines']}  "
                f"line_comment_lines={st['line_comment_lines']}  "
                f"block_comment_open_lines={st['block_comment_open_lines']}"
            )
        else:
            print("  (no single resolved .c include — skipped deep stats)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
