# INSTALL

This file explains how to install dependencies for the `decomp-engine` repository.
The **only** supported portable host is the **SDL/native shell** (`engine/shells/sdl`).

## What You Need

- Python 3.10+
- SCons 4.x
- a C/C++ toolchain
- **SDL3** development headers and libraries
- a legally obtained FireRed ROM for local testing only

## Recommended Windows Setup

The repository is actively used on Windows with MinGW/MSYS2-style GCC.

Install:

1. Python 3
2. MSYS2 / MinGW-w64 GCC toolchain
3. SCons
4. Git
5. SDL3 (for the canonical shell)

### Windows Dependency Install

If you use MSYS2, install the compiler/toolchain pieces with something like:

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils git python
python -m pip install scons
```

If you already have a working MinGW64 environment elsewhere, that is fine too.

## Linux Setup

Install a recent GCC/G++ toolchain, Python 3, Git, and SCons.

Example:

```bash
sudo apt update
sudo apt install build-essential git python3 python3-pip
python3 -m pip install --user scons
```

## macOS Setup

Install Xcode Command Line Tools, Python 3, Git, and SCons.

Example:

```bash
xcode-select --install
python3 -m pip install --user scons
```

## Build Steps

### SDL/native shell (canonical)

```bash
cd engine/shells/sdl
scons -Q platform=windows target=debug -j4
```

The SDL build produces:

- `build/decomp_engine_sdl.exe` (or `decomp_engine_sdl` on Unix)
- `build/runtime_progress_runner.exe` (or `runtime_progress_runner`) — headless core smoke binary
- `build/SDL3.dll` on Windows when `sdl_bin_dir` is set (see below)

The shell defaults to a debug build for stability. For an optimized shell build, pass
`target=release`. See [engine/shells/sdl/README.md](engine/shells/sdl/README.md).

If SDL3 is in a custom location:

```bash
scons sdl_include_dir=/path/to/include sdl_lib_dir=/path/to/SDL3/lib sdl_bin_dir=/path/to/SDL3/bin
```

On Windows, the SDL build copies `SDL3.dll` from `sdl_bin_dir` into `build/`
so `build/decomp_engine_sdl.exe` can run without manually editing `PATH`.

Run:

```bash
build/decomp_engine_sdl.exe baserom.gba
```

Headless core check:

```bash
build/runtime_progress_runner.exe baserom.gba
```

## Runtime Test Requirements

For local runtime checks, place your legally obtained ROM at a local path such as:

```text
baserom.gba
```

That file is ignored by `.gitignore` and should not be committed.

## CI and local portable check

Pull requests run **`.github/workflows/portable_host.yml`**: Ubuntu 24.04, **SDL**
debug build from `engine/shells/sdl` (includes **`runtime_progress_runner`**).

To match locally:

- **Linux / macOS:** `bash tools/verify_portable_default.sh` (set `SDL3_INCLUDE_DIR` /
  `SDL3_LIB_DIR` if SCons cannot find SDL3).
- **Windows (PowerShell):** `tools/verify_portable_default.ps1`

## Optional Notes

- `docs/architecture.txt` explains the engine/core split
- `docs/build.txt` summarizes the current build behavior
- `pokefirered_core/` still exists as a compatibility layer, but canonical ownership
  for the refactored public architecture is now split between `engine/` and `cores/firered/`

## Troubleshooting

### `scons` not found

Install it with pip:

```bash
python -m pip install scons
```

### ROM accidentally shows up in git status

The root `.gitignore` now ignores ROMs, saves, logs, caches, and build outputs.
If a local file is still tracked from older history, remove it from the index before release.
