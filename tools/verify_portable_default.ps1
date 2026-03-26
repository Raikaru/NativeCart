# Match CI: Direction B + Project C block-word offline + layout bins + Direction D, then SDL debug. Requires Python, MinGW GCC, SCons, SDL3.
$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$jobs = if ($env:NUMBER_OF_PROCESSORS) { [int]$env:NUMBER_OF_PROCESSORS } else { 4 }
& python (Join-Path $RepoRoot "tools\check_direction_b_offline.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& python (Join-Path $RepoRoot "tools\check_project_c_block_word_offline.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& python (Join-Path $RepoRoot "tools\check_project_c_phase3_offline.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& python (Join-Path $RepoRoot "tools\validate_map_layout_block_bin.py") --layouts-json (Join-Path $RepoRoot "data\layouts\layouts.json") --repo-root $RepoRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$buildDir = Join-Path $RepoRoot "build"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
& python (Join-Path $RepoRoot "tools\portable_generators\build_map_layout_rom_companion_bundle.py") $RepoRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& python (Join-Path $RepoRoot "tools\portable_generators\layout_rom_companion_emit_embed_env.py") `
  (Join-Path $RepoRoot "build\offline_map_layout_rom_companion_bundle.bin.manifest.txt") -b 0
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& python (Join-Path $RepoRoot "tools\check_direction_d_offline.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Push-Location (Join-Path $RepoRoot "engine\shells\sdl")
try {
    scons -Q platform=windows target=debug -j$jobs
} finally {
    Pop-Location
}
