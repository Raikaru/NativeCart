# Match CI: SDL debug (shell + runtime_progress_runner). Requires MinGW GCC, SCons, SDL3.
$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$jobs = if ($env:NUMBER_OF_PROCESSORS) { [int]$env:NUMBER_OF_PROCESSORS } else { 4 }
Push-Location (Join-Path $RepoRoot "engine\shells\sdl")
try {
    scons -Q platform=windows target=debug -j$jobs
} finally {
    Pop-Location
}
