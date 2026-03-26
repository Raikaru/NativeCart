#!/usr/bin/env python3
"""
Copy a host ROM image and overwrite a byte range with the **map layout ROM companion bundle**
(``build_map_layout_rom_companion_bundle.py`` output).

This is a **testing helper** only: it does not patch pointers, resize ROMs, or define a new format.
The bundle must have been built with ``--metatile-pack-base-offset`` equal to the splice offset **P**
whenever **P != 0** (recorded as ``METATILE_PACK_BASE_OFFSET`` in the manifest). Otherwise metatile
directory ``border_off`` / ``map_off`` values will not match absolute file offsets in the host image.

**Never overwrites the input path** — always writes ``--output``.

Usage (from repo root):
  python tools/portable_generators/layout_rom_companion_splice_bundle.py \\
    --input path/to/base.gba --output path/to/test.gba \\
    --offset 0xP \\
    --bundle build/offline_map_layout_rom_companion_bundle.bin

Options:
  --manifest PATH     default: <bundle>.manifest.txt (same suffix rule as emit helper)
  --no-emit-env       skip ``layout_rom_companion_emit_embed_env.py``
  --validate          run ``validate_map_layout_rom_packs.py`` on the output ROM
  --repo-root PATH    repo root for ``--validate`` layouts path (default: current directory)
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

GEN = Path(__file__).resolve().parent


def parse_manifest(path: Path) -> dict[str, int]:
    out: dict[str, int] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line or "=" not in line:
            continue
        k, v = line.split("=", 1)
        k, v = k.strip(), v.strip()
        try:
            out[k] = int(v.replace("_", ""), 0)
        except ValueError as e:
            raise ValueError(f"{path}: bad line {raw!r} ({e})") from e
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--input", type=Path, required=True, help="source ROM / host image (read-only)")
    ap.add_argument("--output", type=Path, required=True, help="copy written here; bundle overlaid at --offset")
    ap.add_argument(
        "--offset",
        type=lambda s: int(s.replace("_", ""), 0),
        required=True,
        help="file offset P where the first byte of the bundle is written (hex or decimal)",
    )
    ap.add_argument("--bundle", type=Path, required=True, help="companion bundle .bin")
    ap.add_argument("--manifest", type=Path, default=None, help="manifest path (default: bundle + .manifest.txt)")
    ap.add_argument("--no-emit-env", action="store_true", help="do not print embed env via emit helper")
    ap.add_argument("--validate", action="store_true", help="run validate_map_layout_rom_packs.py on output")
    ap.add_argument(
        "--repo-root",
        type=Path,
        default=None,
        help="repo root for layouts.json when using --validate (default: cwd)",
    )
    args = ap.parse_args()

    inp = args.input.resolve()
    out = args.output.resolve()
    bundle_path = args.bundle.resolve()
    if not inp.is_file():
        print(f"missing input {inp}", file=sys.stderr)
        return 1
    if not bundle_path.is_file():
        print(f"missing bundle {bundle_path}", file=sys.stderr)
        return 1

    man_path = args.manifest.resolve() if args.manifest is not None else bundle_path.with_suffix(bundle_path.suffix + ".manifest.txt")
    if not man_path.is_file():
        print(f"missing manifest {man_path}", file=sys.stderr)
        return 1

    try:
        d = parse_manifest(man_path)
    except ValueError as e:
        print(str(e), file=sys.stderr)
        return 2

    need = (
        "METATILES_OFF",
        "DIMENSIONS_OFF",
        "TILESET_INDICES_OFF",
        "METATILES_BYTES",
        "DIMENSIONS_BYTES",
        "TILESET_INDICES_BYTES",
        "TOTAL_BYTES",
    )
    for k in need:
        if k not in d:
            print(f"manifest missing {k}", file=sys.stderr)
            return 1

    built_base = d.get("METATILE_PACK_BASE_OFFSET", 0)
    p = args.offset
    if p < 0:
        print("--offset must be >= 0", file=sys.stderr)
        return 2
    if p != built_base:
        print(
            f"error: splice --offset {hex(p)} does not match manifest METATILE_PACK_BASE_OFFSET "
            f"{hex(built_base)}.\n"
            "Rebuild the bundle with:\n"
            "  python tools/portable_generators/build_map_layout_rom_companion_bundle.py <root> <out.bin> \\\n"
            f"    --metatile-pack-base-offset {hex(p)}\n"
            "(Legacy manifests without METATILE_PACK_BASE_OFFSET are treated as base 0.)",
            file=sys.stderr,
        )
        return 2

    bundle = bundle_path.read_bytes()
    if len(bundle) != d["TOTAL_BYTES"]:
        print(
            f"error: bundle file is {len(bundle)} bytes but manifest TOTAL_BYTES={d['TOTAL_BYTES']}",
            file=sys.stderr,
        )
        return 2

    rom = bytearray(inp.read_bytes())
    end = p + len(bundle)
    if end > len(rom):
        print(
            f"error: bundle ends at file offset {hex(end)} but host image is only {len(rom)} bytes",
            file=sys.stderr,
        )
        return 2

    rom[p:end] = bundle
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(rom)
    print(f"Wrote {out} ({len(rom)} bytes) with bundle ({len(bundle)} bytes) at {hex(p)}")

    py = sys.executable
    if not args.no_emit_env:
        cmd = [
            py,
            str(GEN / "layout_rom_companion_emit_embed_env.py"),
            str(man_path),
            "-b",
            hex(p),
            "--rom",
            str(out),
        ]
        print("+", " ".join(cmd), flush=True)
        r = subprocess.run(cmd, cwd=str((args.repo_root or Path.cwd()).resolve()))
        if r.returncode != 0:
            return r.returncode

    if args.validate:
        root = (args.repo_root if args.repo_root is not None else Path.cwd()).resolve()
        layouts = root / "data" / "layouts" / "layouts.json"
        if not layouts.is_file():
            print(f"--validate: missing {layouts} (set --repo-root?)", file=sys.stderr)
            return 1
        abs_meta = p + d["METATILES_OFF"]
        abs_dim = p + d["DIMENSIONS_OFF"]
        abs_ts = p + d["TILESET_INDICES_OFF"]
        vcmd = [
            py,
            str(GEN / "validate_map_layout_rom_packs.py"),
            str(out),
            "--metatiles-off",
            hex(abs_meta),
            "--dimensions-off",
            hex(abs_dim),
            "--tileset-indices-off",
            hex(abs_ts),
            "--layouts-json",
            str(layouts),
            "--validate-block-words",
        ]
        print("+", " ".join(vcmd), flush=True)
        return subprocess.run(vcmd, cwd=str(root)).returncode

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
