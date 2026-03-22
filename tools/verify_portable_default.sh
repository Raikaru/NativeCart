#!/usr/bin/env bash
# Match CI: SDL debug build (includes decomp_engine_sdl + runtime_progress_runner).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/engine/shells/sdl"
scons -Q target=debug -j"${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
