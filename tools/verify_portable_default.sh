#!/usr/bin/env bash
# Match CI: Direction B + Project C block-word offline + layout bins + Direction D, then SDL debug build.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python3 "$ROOT/tools/check_direction_b_offline.py"
python3 "$ROOT/tools/check_project_c_block_word_offline.py"
python3 "$ROOT/tools/check_project_c_phase3_offline.py"
python3 "$ROOT/tools/validate_map_layout_block_bin.py" --layouts-json "$ROOT/data/layouts/layouts.json" --repo-root "$ROOT"
mkdir -p "$ROOT/build"
python3 "$ROOT/tools/portable_generators/build_map_layout_rom_companion_bundle.py" "$ROOT"
python3 "$ROOT/tools/portable_generators/layout_rom_companion_emit_embed_env.py" \
  "$ROOT/build/offline_map_layout_rom_companion_bundle.bin.manifest.txt" -b 0
python3 "$ROOT/tools/check_direction_d_offline.py"
cd "$ROOT/engine/shells/sdl"
scons -Q target=debug -j"${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
