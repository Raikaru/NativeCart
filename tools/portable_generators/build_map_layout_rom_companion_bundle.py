#!/usr/bin/env python3
"""
Build the three portable layout companion blobs, concatenate them in a fixed order, write a
manifest, and validate the bundle with **one** ``validate_map_layout_rom_packs.py`` invocation.

**Layout (concatenation order):**
  1. Metatile directory + border/map blobs (``build_map_layout_metatiles_rom_pack.py``)
  2. Dimensions table (``build_map_layout_dimensions_rom_pack.py``)
  3. Tileset-indices table (``build_map_layout_tileset_indices_rom_pack.py``)

**Per-part outputs** (same paths as offline gates / ad hoc workflows):
  - ``build/offline_map_layout_metatiles_pack.bin``
  - ``build/offline_map_layout_dimensions_pack.bin``
  - ``build/offline_map_layout_tileset_indices_pack.bin``

**Bundle output:** ``<out_bundle>`` (default ``build/offline_map_layout_rom_companion_bundle.bin``)

**Manifest:** ``<out_bundle>.manifest.txt`` — file offsets **relative to the start of the bundle**
(hex ``METATILES_OFF``, ``DIMENSIONS_OFF``, ``TILESET_INDICES_OFF``) plus byte sizes; and
``METATILE_PACK_BASE_OFFSET`` (same value passed to ``build_map_layout_metatiles_rom_pack.py``
``--pack-base-offset``). Splice helpers require ``METATILE_PACK_BASE_OFFSET == P`` when embedding at **P**.

Usage:
  python build_map_layout_rom_companion_bundle.py <pokefirered_root> [out_bundle.bin]

Options:
  --metatile-pack-base-offset P   passed to metatile builder (default 0); must match host splice offset
  --no-validate   build + manifest only (for debugging)

When P != 0, post-build validation uses a temporary **P-byte zero pad + bundle** image so metatile
absolute u32 blob offsets resolve like a host ROM with the bundle starting at P.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
GEN = REPO / "tools" / "portable_generators"

META_NAME = "offline_map_layout_metatiles_pack.bin"
DIM_NAME = "offline_map_layout_dimensions_pack.bin"
TS_NAME = "offline_map_layout_tileset_indices_pack.bin"
DEFAULT_BUNDLE = "offline_map_layout_rom_companion_bundle.bin"


def run_builder(py: str, script: str, root: Path, out: Path, extra: list[str] | None = None) -> None:
    cmd = [py, str(GEN / script), str(root), str(out)]
    if extra:
        cmd.extend(extra)
    print("+", " ".join(cmd), flush=True)
    r = subprocess.run(cmd, cwd=str(root))
    if r.returncode != 0:
        raise SystemExit(r.returncode)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("root", type=Path, help="PokeFireRed repo root")
    ap.add_argument(
        "bundle",
        type=Path,
        nargs="?",
        default=None,
        help=f"combined output (default build/{DEFAULT_BUNDLE})",
    )
    ap.add_argument("--no-validate", action="store_true", help="skip validate_map_layout_rom_packs.py")
    ap.add_argument(
        "--metatile-pack-base-offset",
        type=lambda s: int(s.replace("_", ""), 0),
        default=0,
        help="host file offset of metatile pack base when embedded (passed to metatile builder; default 0)",
    )
    args = ap.parse_args()
    root = args.root.resolve()
    py = sys.executable
    build_dir = root / "build"
    build_dir.mkdir(parents=True, exist_ok=True)

    meta_path = build_dir / META_NAME
    dim_path = build_dir / DIM_NAME
    ts_path = build_dir / TS_NAME
    bundle_path = args.bundle.resolve() if args.bundle is not None else build_dir / DEFAULT_BUNDLE

    meta_extra: list[str] | None = None
    if args.metatile_pack_base_offset != 0:
        meta_extra = ["--pack-base-offset", hex(args.metatile_pack_base_offset)]
    run_builder(py, "build_map_layout_metatiles_rom_pack.py", root, meta_path, meta_extra)
    run_builder(py, "build_map_layout_dimensions_rom_pack.py", root, dim_path)
    run_builder(py, "build_map_layout_tileset_indices_rom_pack.py", root, ts_path)

    meta = meta_path.read_bytes()
    dim = dim_path.read_bytes()
    ts = ts_path.read_bytes()
    bundle = meta + dim + ts
    bundle_path.parent.mkdir(parents=True, exist_ok=True)
    bundle_path.write_bytes(bundle)

    dim_off = len(meta)
    ts_off = dim_off + len(dim)
    man_path = bundle_path.with_suffix(bundle_path.suffix + ".manifest.txt")
    mpb = args.metatile_pack_base_offset
    lines = [
        "# Map layout ROM companion bundle (metatile pack + dimensions + tileset indices)",
        "# Offsets are relative to the first byte of the bundle file.",
        f"METATILES_OFF=0x0",
        f"METATILES_BYTES={len(meta)}",
        f"DIMENSIONS_OFF={hex(dim_off)}",
        f"DIMENSIONS_BYTES={len(dim)}",
        f"TILESET_INDICES_OFF={hex(ts_off)}",
        f"TILESET_INDICES_BYTES={len(ts)}",
        f"TOTAL_BYTES={len(bundle)}",
        f"METATILE_PACK_BASE_OFFSET={hex(mpb)}",
        "",
        "# Example validate (from repo root):",
        "# python tools/portable_generators/validate_map_layout_rom_packs.py \\",
        f"#   {bundle_path.relative_to(root)} \\",
        "#   --metatiles-off 0x0 \\",
        f"#   --dimensions-off {hex(dim_off)} \\",
        f"#   --tileset-indices-off {hex(ts_off)} \\",
        "#   --layouts-json data/layouts/layouts.json --validate-block-words",
        "",
        "# FIRERED_ROM_* env + inspect commands (bundle at file offset 0 in test ROM):",
        "# python tools/portable_generators/layout_rom_companion_emit_embed_env.py \\",
        f"#   {man_path.relative_to(root)} -b 0",
        "",
        "# Splice copy (offset must match METATILE_PACK_BASE_OFFSET):",
        "# python tools/portable_generators/layout_rom_companion_splice_bundle.py \\",
        "#   --input path/to/base.gba --output path/to/test.gba \\",
        f"#   --offset {hex(mpb)} --bundle {bundle_path.relative_to(root)} --validate",
        "",
    ]
    man_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {len(bundle)} bytes to {bundle_path}")
    print(f"Wrote manifest {man_path}")

    if args.no_validate:
        return 0

    # Metatile directory u32s are absolute host offsets (pack base P). Validating the bare bundle
    # file only matches that layout when P == 0; for P != 0 use a padded image (P zero bytes + bundle).
    val_path = bundle_path
    abs_meta = mpb
    abs_dim = mpb + dim_off
    abs_ts = mpb + ts_off
    pad_path: Path | None = None
    if mpb != 0:
        pad_path = build_dir / ".offline_companion_validate_padded_image.bin"
        pad_path.write_bytes(bytes(mpb) + bundle)
        val_path = pad_path

    cmd = [
        py,
        str(GEN / "validate_map_layout_rom_packs.py"),
        str(val_path),
        "--metatiles-off",
        hex(abs_meta),
        "--dimensions-off",
        hex(abs_dim),
        "--tileset-indices-off",
        hex(abs_ts),
        "--layouts-json",
        str(root / "data/layouts/layouts.json"),
        "--validate-block-words",
    ]
    print("+", " ".join(cmd), flush=True)
    try:
        code = subprocess.run(cmd, cwd=str(root)).returncode
    finally:
        if pad_path is not None:
            try:
                pad_path.unlink()
            except OSError:
                pass
    return code


if __name__ == "__main__":
    raise SystemExit(main())
