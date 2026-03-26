#!/usr/bin/env python3
"""
One-shot **layout companion test ROM** prep: build the bundle, splice it into a **copy** of a host
ROM at file offset **P**, then emit **FIRERED_ROM_MAP_LAYOUT_*_OFF** (and optional inspect/validate
snippets) using the same **P**.

Delegates to existing tools only (no duplicated manifest/env formatting):

1. ``build_map_layout_rom_companion_bundle.py`` with ``--metatile-pack-base-offset P``
2. ``layout_rom_companion_splice_bundle.py`` (copy host → output, overlay bundle)
3. ``layout_rom_companion_emit_embed_env.py`` for env + example ``rom_blob_inspect`` / validate lines

The input ROM is never modified.

Before picking **P** on a retail-sized ROM, run **`layout_rom_companion_audit_rom_placement.py`** (read-only bounds + slice stats; optional trailing-padding heuristic) and **`tools/rom_blob_inspect.py`** on the candidate range — see **`docs/portable_rom_map_layout_metatiles.md`**.

Usage (repo root as cwd):

  python tools/portable_generators/layout_rom_companion_prepare_test_rom.py \\
    --input-rom path/to/base.gba --output-rom path/to/test.gba --offset 0xP [--validate]

Options:
  --repo-root PATH     PokeFireRed root (default: current working directory)
  --bundle-out PATH    bundle output (default: build/offline_map_layout_rom_companion_bundle.bin)
  --validate           after splice, run ``validate_map_layout_rom_packs.py`` on the output ROM
  --bash-only / --powershell-only / --inspect-only
                       forwarded to ``layout_rom_companion_emit_embed_env.py`` (mutually exclusive)
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

GEN = Path(__file__).resolve().parent


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--input-rom", type=Path, required=True)
    ap.add_argument("--output-rom", type=Path, required=True)
    ap.add_argument(
        "--offset",
        type=lambda s: int(s.replace("_", ""), 0),
        required=True,
        help="file offset P where the bundle starts in the output ROM (hex or decimal)",
    )
    ap.add_argument("--repo-root", type=Path, default=None, help="repo root (default: cwd)")
    ap.add_argument(
        "--bundle-out",
        type=Path,
        default=None,
        help="companion bundle path written by the builder (default: build/offline_map_layout_rom_companion_bundle.bin)",
    )
    ap.add_argument(
        "--validate",
        action="store_true",
        help="run layout pack validator on the spliced output ROM",
    )
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--bash-only", action="store_true")
    g.add_argument("--powershell-only", action="store_true")
    g.add_argument("--inspect-only", action="store_true")
    args = ap.parse_args()

    root = (args.repo_root if args.repo_root is not None else Path.cwd()).resolve()
    inp = args.input_rom.resolve()
    out = args.output_rom.resolve()
    if inp == out:
        print("error: --input-rom and --output-rom must differ (copy-only workflow)", file=sys.stderr)
        return 2
    if not inp.is_file():
        print(f"missing --input-rom {inp}", file=sys.stderr)
        return 1

    p = args.offset
    if p < 0:
        print("error: --offset must be >= 0", file=sys.stderr)
        return 2

    bundle = (
        args.bundle_out.resolve()
        if args.bundle_out is not None
        else (root / "build" / "offline_map_layout_rom_companion_bundle.bin").resolve()
    )
    manifest = bundle.with_suffix(bundle.suffix + ".manifest.txt")

    py = sys.executable
    build_cmd = [
        py,
        str(GEN / "build_map_layout_rom_companion_bundle.py"),
        str(root),
        str(bundle),
        "--metatile-pack-base-offset",
        hex(p),
    ]
    print("+", " ".join(build_cmd), flush=True)
    r = subprocess.run(build_cmd, cwd=str(root))
    if r.returncode != 0:
        return r.returncode

    splice_cmd = [
        py,
        str(GEN / "layout_rom_companion_splice_bundle.py"),
        "--input",
        str(inp),
        "--output",
        str(out),
        "--offset",
        hex(p),
        "--bundle",
        str(bundle),
        "--repo-root",
        str(root),
        "--no-emit-env",
    ]
    if args.validate:
        splice_cmd.append("--validate")
    print("+", " ".join(splice_cmd), flush=True)
    r = subprocess.run(splice_cmd, cwd=str(root))
    if r.returncode != 0:
        return r.returncode

    emit_cmd = [
        py,
        str(GEN / "layout_rom_companion_emit_embed_env.py"),
        str(manifest),
        "-b",
        hex(p),
        "--rom",
        str(out),
    ]
    if args.bash_only:
        emit_cmd.append("--bash-only")
    elif args.powershell_only:
        emit_cmd.append("--powershell-only")
    elif args.inspect_only:
        emit_cmd.append("--inspect-only")
    print("+", " ".join(emit_cmd), flush=True)
    r = subprocess.run(emit_cmd, cwd=str(root))
    if r.returncode != 0:
        return r.returncode

    print()
    print("# --- layout_rom_companion_prepare_test_rom (summary) ---")
    print(f"# output_rom: {out}")
    print(f"# bundle:     {bundle}")
    print(f"# manifest:   {manifest}")
    print(f"# offset P:   {hex(p)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
