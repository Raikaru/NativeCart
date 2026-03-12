# INSTALL

This file explains how to install the dependencies needed to build the current
public `decomp-engine` repository and its FireRed Godot shell.

## What You Need

At minimum, the current build path needs:

- Python 3.10+
- SCons 4.x
- a C/C++ toolchain
- [godot-cpp](https://github.com/godotengine/godot-cpp) installed separately as an external dependency
- a Godot 4.x editor/runtime for manual testing
- a legally obtained FireRed ROM for local testing only

Optional for the SDL shell:

- SDL2 development headers and libraries

## Recommended Windows Setup

The repository is actively used on Windows with MinGW/MSYS2-style GCC.

Install:

1. Python 3
2. MSYS2 / MinGW-w64 GCC toolchain
3. SCons
4. Git
5. Godot 4.x

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

## Clone Required Dependencies

`godot-cpp` is not committed to this repository. Install it locally.

Recommended default: clone it at the repository root so the current SCons setup finds it automatically.

```bash
git clone https://github.com/godotengine/godot-cpp.git
```

The current SCons build expects `godot-cpp` by default at:

```text
./godot-cpp
```

If you keep it elsewhere, pass `godot_cpp_path=/path/to/godot-cpp` to SCons.

## Build Steps

### 1. Build the Godot extension

```bash
cd gdextension
scons -Q platform=windows target=debug -j4
```

Adjust `platform` and parallelism as needed for your machine.

Examples:

```bash
scons -Q platform=linux target=debug -j4
scons -Q platform=macos target=debug -j4
scons -Q platform=windows target=release -j4
```

### 2. Output locations

Main outputs:

- `gdextension/bin/libpokefirered.debug.windows.x86_64.dll`
- `build/libfirered_core.a`
- `build/runtime_progress_runner.exe`

## Optional SDL Shell Build

The repository also includes a standalone SDL shell in `engine/shells/sdl/`.

Build it with:

```bash
cd engine/shells/sdl
scons -Q platform=windows target=debug -j4
```

If SDL2 is installed in a custom location, pass:

```bash
scons sdl_include_dir=/path/to/include sdl_lib_dir=/path/to/SDL2/lib
```

Run it with:

```bash
build/decomp_engine_sdl.exe baserom.gba
```

## Runtime Test Requirements

For local runtime checks, place your legally obtained ROM at a local path such as:

```text
baserom.gba
```

That file is ignored by `.gitignore` and should not be committed.

## Minimal Verification

After building, a lightweight runtime verification command is:

```bash
build/runtime_progress_runner.exe baserom.gba
```

That confirms the FireRed core still boots and frame-steps through the engine split.

## Godot Manual Testing

Open the project in Godot 4.x and run the smoke/manual scene from the repository.

If your Godot executable is not on `PATH`, launch it directly and open this repo as a project.

## Optional Notes

- `docs/architecture.txt` explains the engine/core split
- `docs/build.txt` summarizes the current build behavior
- `pokefirered_core/` still exists as a compatibility layer, but canonical ownership
  for the refactored public architecture is now split between `engine/` and `cores/firered/`

## Troubleshooting

### `godot-cpp library not found`

Make sure `godot-cpp/` exists at the repository root, or pass a custom path:

```bash
scons godot_cpp_path=/absolute/path/to/godot-cpp
```

### `scons` not found

Install it with pip:

```bash
python -m pip install scons
```

### ROM accidentally shows up in git status

The root `.gitignore` now ignores ROMs, saves, logs, caches, and build outputs.
If a local file is still tracked from older history, remove it from the index before release.
