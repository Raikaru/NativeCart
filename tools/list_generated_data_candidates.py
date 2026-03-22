#!/usr/bin/env python3
"""
Read-only scan: list .c/.h under pokefirered_core/generated with line counts.

Helps prioritize ROM seam / layout analysis (e.g. map_data_portable.c, wild_encounters.h)
without opening huge files blindly. See docs/generated_data_rom_seam_playbook.md.

Examples:
  python tools/list_generated_data_candidates.py
  python tools/list_generated_data_candidates.py --min-lines 10000
  python tools/list_generated_data_candidates.py --root pokefirered_core/generated/src/data
  python tools/list_generated_data_candidates.py --grep wild
  python tools/list_generated_data_candidates.py --grep map --json
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def count_lines(path: Path) -> int:
    n = 0
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            n += chunk.count(b"\n")
    return n


def rel_under(root: Path, path: Path) -> Path:
    try:
        return path.relative_to(root)
    except ValueError:
        return path


def main() -> int:
    repo = Path(__file__).resolve().parent.parent
    p = argparse.ArgumentParser(
        description="List .c/.h files under generated tree sorted by line count (read-only)."
    )
    p.add_argument(
        "--root",
        type=Path,
        default=repo / "pokefirered_core" / "generated",
        help="Directory to scan (default: pokefirered_core/generated)",
    )
    p.add_argument(
        "--min-lines",
        type=int,
        default=0,
        metavar="N",
        help="Only print files with at least N lines (default: 0)",
    )
    p.add_argument(
        "--grep",
        metavar="SUBSTR",
        help="Only list files whose path relative to --root contains SUBSTR (case-insensitive)",
    )
    p.add_argument(
        "--json",
        action="store_true",
        help="Print a JSON array of {lines, path} objects (path uses forward slashes)",
    )
    args = p.parse_args()
    root: Path = args.root
    if not root.is_dir():
        print(f"error: not a directory: {root}", file=sys.stderr)
        return 2

    exts = {".c", ".h"}
    rows: list[tuple[int, Path]] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in exts:
            continue
        try:
            n = count_lines(path)
        except OSError as e:
            print(f"warning: skip {path}: {e}", file=sys.stderr)
            continue
        if n >= args.min_lines:
            rows.append((n, path))

    needle = (args.grep or "").lower()
    if needle:
        rows = [
            (n, path)
            for n, path in rows
            if needle in str(rel_under(root, path)).lower()
        ]

    rows.sort(key=lambda t: (-t[0], str(t[1]).lower()))
    if args.json:
        payload = [
            {
                "lines": n,
                "path": str(rel_under(root, path)).replace("\\", "/"),
            }
            for n, path in rows
        ]
        json.dump(payload, sys.stdout, indent=2)
        sys.stdout.write("\n")
    else:
        print(f"root={root} files={len(rows)} (extensions={sorted(exts)})")
        for n, path in rows:
            rel = rel_under(root, path)
            print(f"{n:8d}  {rel}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
