#!/usr/bin/env python3
"""
Offline validation for layout **map.bin** / **border.bin** files.

**Cell width 2 (default):** little-endian **`u16`** PRET **wire V1** (10-bit metatile field in low bits).

**Cell width 4 (Phase 3 tooling):** little-endian **`u32`** per cell — **`MAP_GRID_BLOCK_WORD_PHASE3_U32_*`**
in `include/constants/map_grid_block_word.h` (16-bit metatile id, 2-bit collision, 3-bit elevation).
Vanilla repo bins are **u16**; use `--cell-width 4` only when your pipeline emits **`u32`** layout grids.

Modes:
  * Single file: path + optional `--expected-words` (= number of **cells** = width*height or border w*h).
  * Repo sweep: `--layouts-json` (+ `--repo-root`) checks every layout entry.

Example:
  python tools/validate_map_layout_block_bin.py data/layouts/LAYOUT/map.bin --expected-words 400
  python tools/validate_map_layout_block_bin.py --layouts-json data/layouts/layouts.json --repo-root .
  python tools/validate_map_layout_block_bin.py hack/u32/map.bin --cell-width 4 --expected-words 400
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

_PGEN = Path(__file__).resolve().parent / "portable_generators"
if str(_PGEN) not in sys.path:
    sys.path.insert(0, str(_PGEN))
from map_visual_policy_constants import (
    DEFAULT_PHASE3_U32_METATILE_ID_LIMIT_EXCLUSIVE,
    DEFAULT_WIRE_V1_METATILE_ID_LIMIT_EXCLUSIVE,
    MAP_GRID_BLOCK_WORD_PHASE3_U32_METATILE_ID_MASK,
    MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK,
)


def validate_words(
    data: bytes,
    *,
    expected_cells: int | None,
    metatile_limit_excl: int,
    cell_width: int,
) -> list[str]:
    errs: list[str] = []
    if cell_width == 2:
        mask = MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK
        if len(data) % 2 != 0:
            errs.append(f"byte length {len(data)} is not a multiple of 2")
            return errs
        n = len(data) // 2
        stride = 2
    elif cell_width == 4:
        mask = MAP_GRID_BLOCK_WORD_PHASE3_U32_METATILE_ID_MASK
        if len(data) % 4 != 0:
            errs.append(f"byte length {len(data)} is not a multiple of 4 (u32 cells)")
            return errs
        n = len(data) // 4
        stride = 4
    else:
        return [f"unsupported cell_width {cell_width}"]

    if expected_cells is not None and n != expected_cells:
        errs.append(f"cell count {n} != expected {expected_cells}")
    if metatile_limit_excl < 1 or metatile_limit_excl > mask + 1:
        errs.append(f"metatile-id-limit must be 1..{mask + 1} (exclusive max)")
        return errs

    for i in range(n):
        off = i * stride
        if stride == 2:
            w = int.from_bytes(data[off : off + 2], "little")
            fmt = "04X"
        else:
            w = int.from_bytes(data[off : off + 4], "little")
            fmt = "08X"
        mid = w & mask
        if mid >= metatile_limit_excl:
            errs.append(f"cell[{i}]=0x{w:{fmt}} metatile_id={mid} >= limit {metatile_limit_excl}")
            if len(errs) >= 20:
                errs.append("… (further errors omitted)")
                break
    return errs


def validate_layout_bin_file(
    path: Path,
    *,
    expected_cells: int,
    metatile_limit_excl: int,
    kind: str,
    cell_width: int,
) -> list[str]:
    if expected_cells == 0:
        if not path.is_file():
            return []
        data = path.read_bytes()
        return validate_words(
            data,
            expected_cells=None,
            metatile_limit_excl=metatile_limit_excl,
            cell_width=cell_width,
        )

    if not path.is_file():
        return [f"{kind}: missing file {path}"]

    data = path.read_bytes()
    return validate_words(
        data,
        expected_cells=expected_cells,
        metatile_limit_excl=metatile_limit_excl,
        cell_width=cell_width,
    )


def validate_all_from_layouts_json(
    layouts_json: Path,
    repo_root: Path,
    *,
    metatile_limit_excl: int,
    cell_width: int,
) -> int:
    layouts_json = layouts_json.resolve()
    repo_root = repo_root.resolve()
    try:
        doc = json.loads(layouts_json.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as e:
        print(f"{layouts_json}: {e}", file=sys.stderr)
        return 2

    layouts = doc.get("layouts")
    if not isinstance(layouts, list):
        print("layouts.json: missing or invalid `layouts` array", file=sys.stderr)
        return 2

    exit_code = 0
    n_ok = 0
    n_skipped = 0
    for entry in layouts:
        if not isinstance(entry, dict):
            n_skipped += 1
            continue
        required = (
            "width",
            "height",
            "border_width",
            "border_height",
            "blockdata_filepath",
            "border_filepath",
        )
        if not all(k in entry for k in required):
            n_skipped += 1
            continue
        lid = entry.get("id", "?")
        try:
            w = int(entry["width"])
            h = int(entry["height"])
            bw = int(entry["border_width"])
            bh = int(entry["border_height"])
            map_rel = entry["blockdata_filepath"]
            border_rel = entry["border_filepath"]
        except (KeyError, TypeError, ValueError) as e:
            print(f"layout {lid}: bad entry ({e})", file=sys.stderr)
            exit_code = 1
            continue

        map_cells = w * h
        border_cells = bw * bh
        map_path = (repo_root / map_rel).resolve()
        border_path = (repo_root / border_rel).resolve()

        for path, exp, kind in (
            (map_path, map_cells, "map.bin"),
            (border_path, border_cells, "border.bin"),
        ):
            errs = validate_layout_bin_file(
                path,
                expected_cells=exp,
                metatile_limit_excl=metatile_limit_excl,
                kind=kind,
                cell_width=cell_width,
            )
            if errs:
                exit_code = 1
                for e in errs:
                    print(f"layout {lid} ({kind}) {path}: {e}", file=sys.stderr)
            else:
                n_ok += 1

    if exit_code == 0:
        n_layouts = len(layouts) - n_skipped
        cw = f"u{cell_width * 8}"
        print(
            f"OK: {layouts_json.name} - {n_ok} bin file(s) validated ({cw} cells) "
            f"({n_layouts} layout entries, {n_skipped} skipped placeholders)."
        )
    return exit_code


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "bin_path",
        type=Path,
        nargs="?",
        default=None,
        help="single map.bin or border.bin (omit when using --layouts-json)",
    )
    ap.add_argument(
        "--layouts-json",
        type=Path,
        default=None,
        help="validate all layout map/border bins listed in this file",
    )
    ap.add_argument(
        "--repo-root",
        type=Path,
        default=Path("."),
        help="root for relative paths in layouts.json (default: current directory)",
    )
    ap.add_argument(
        "--cell-width",
        type=int,
        choices=(2, 4),
        default=2,
        help="2 = u16 PRET wire V1 (default); 4 = Phase 3 u32 cell",
    )
    ap.add_argument(
        "--expected-words",
        type=int,
        default=None,
        help="if set, must equal number of cells (len//cell_width in bytes; single-file mode only)",
    )
    ap.add_argument(
        "--metatile-id-limit",
        type=int,
        default=None,
        metavar="EXCLUSIVE_MAX",
        help="exclusive max metatile id (default: 1024 for u16 wire V1, 65536 for u32 Phase 3)",
    )
    args = ap.parse_args()

    cell_width = args.cell_width
    if args.metatile_id_limit is not None:
        limit = args.metatile_id_limit
    elif cell_width == 4:
        limit = DEFAULT_PHASE3_U32_METATILE_ID_LIMIT_EXCLUSIVE
    else:
        limit = DEFAULT_WIRE_V1_METATILE_ID_LIMIT_EXCLUSIVE

    mask = (
        MAP_GRID_BLOCK_WORD_PHASE3_U32_METATILE_ID_MASK
        if cell_width == 4
        else MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK
    )

    if args.layouts_json is not None:
        if args.bin_path is not None:
            print("pass either a single bin path or --layouts-json, not both", file=sys.stderr)
            return 2
        return validate_all_from_layouts_json(
            args.layouts_json,
            args.repo_root,
            metatile_limit_excl=limit,
            cell_width=cell_width,
        )

    if args.bin_path is None:
        ap.print_help()
        print("\nerror: bin_path required unless --layouts-json is set", file=sys.stderr)
        return 2

    p = args.bin_path
    if not p.is_file():
        print(f"not a file: {p}", file=sys.stderr)
        return 2
    data = p.read_bytes()
    errs = validate_words(
        data,
        expected_cells=args.expected_words,
        metatile_limit_excl=limit,
        cell_width=cell_width,
    )
    if errs:
        for e in errs:
            print(f"{p}: {e}", file=sys.stderr)
        return 1
    ncells = len(data) // cell_width
    kind = "u16 wire V1" if cell_width == 2 else "u32 Phase 3"
    print(f"OK: {p} ({ncells} cells, {kind}; metatile id < {limit}, mask {mask:#x}).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
