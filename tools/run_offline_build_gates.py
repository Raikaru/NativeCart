#!/usr/bin/env python3
"""
Run offline merge gates when **`make`** / **agbcc** are unavailable (e.g. Windows without devkit).

Equivalent to **`make check-direction-d`** plus an explicit list (see **`Makefile`** `check-offline-gates`).

Does not build **`.gba`** — use Linux/MSYS2 + devkitARM for **`make compare_*`**.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def run(cmd: list[str]) -> int:
    print("+", " ".join(cmd), flush=True)
    r = subprocess.run(cmd, cwd=str(ROOT))
    return r.returncode


def main() -> int:
    py = sys.executable
    steps: list[list[str]] = [
        [py, "tools/check_direction_b_offline.py"],
        [py, "tools/check_project_c_block_word_offline.py"],
        [py, "tools/check_project_c_phase3_offline.py"],
        [py, "tools/validate_map_layout_block_bin.py", "--layouts-json", "data/layouts/layouts.json", "--repo-root", "."],
        [
            py,
            "-c",
            "from pathlib import Path; p=Path('build/offline_splice_test_host.bin'); "
            "p.parent.mkdir(parents=True, exist_ok=True); "
            "p.write_bytes(bytes(0x80000))",
        ],
        [
            py,
            "tools/portable_generators/layout_rom_companion_prepare_test_rom.py",
            "--input-rom",
            "build/offline_splice_test_host.bin",
            "--output-rom",
            "build/offline_splice_test_out.bin",
            "--offset",
            "0",
            "--validate",
            "--repo-root",
            ".",
        ],
        [py, "tools/build_offline_layout_prose_fixture.py", "--repo-root", "."],
        [
            py,
            "tools/portable_generators/layout_rom_companion_audit_rom_placement.py",
            "--rom",
            "build/offline_splice_test_out.bin",
            "--offset",
            "0",
            "--bundle",
            "build/offline_map_layout_rom_companion_bundle.bin",
        ],
        [py, "tools/check_direction_d_offline.py"],
        [py, "tools/validate_field_map_logical_to_physical_inc.py"],
    ]
    for cmd in steps:
        code = run(cmd)
        if code != 0:
            return code
    print("OK: all offline build gates passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
