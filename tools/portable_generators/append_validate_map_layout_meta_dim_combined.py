#!/usr/bin/env python3
"""
Concatenate offline metatile + dimensions layout packs and run validate_map_layout_rom_packs.py
with matching offsets (cross-checks metatile word counts vs dimensions rows).

For the **full** three-part layout companion stack (metatile + dimensions + tileset indices), prefer
``build_map_layout_rom_companion_bundle.py`` (writes per-part bins, bundle, manifest, and validates).

Expects:
  build/offline_map_layout_metatiles_pack.bin
  build/offline_map_layout_dimensions_pack.bin

Writes:
  build/offline_map_layout_metatile_dim_combined.bin
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    py = sys.executable
    meta_path = ROOT / "build" / "offline_map_layout_metatiles_pack.bin"
    dim_path = ROOT / "build" / "offline_map_layout_dimensions_pack.bin"
    if not meta_path.is_file() or not dim_path.is_file():
        print("append_validate_map_layout_meta_dim_combined: missing build/*.bin inputs", file=sys.stderr)
        return 1
    meta = meta_path.read_bytes()
    dim = dim_path.read_bytes()
    comb = ROOT / "build" / "offline_map_layout_metatile_dim_combined.bin"
    comb.write_bytes(meta + dim)
    dim_off = hex(len(meta))
    cmd = [
        py,
        str(ROOT / "tools/portable_generators/validate_map_layout_rom_packs.py"),
        str(comb),
        "--metatiles-off",
        "0x0",
        "--dimensions-off",
        dim_off,
        "--layouts-json",
        str(ROOT / "data/layouts/layouts.json"),
        "--validate-block-words",
    ]
    print("+", " ".join(cmd), flush=True)
    return subprocess.run(cmd, cwd=str(ROOT)).returncode


if __name__ == "__main__":
    raise SystemExit(main())
