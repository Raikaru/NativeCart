# SDL Shell

This directory contains a standalone SDL shell for the generic engine/core +
`cores/firered/` runtime path.

Current scope:

- loads a FireRed ROM through `GameCore`
- steps frames through `engine/core/`
- displays the RGBA framebuffer in an SDL window
- forwards keyboard input to the GBA button bitmask
- exposes simple hotkeys for start-menu and save/load state

## Controls

- arrows = D-pad
- `Z` = A
- `X` = B
- `Enter` = Start
- `Backspace` = Select
- `A` = L
- `S` = R
- `F1` = open start menu
- `F5` = save state
- `F9` = load state
- `Esc` = quit

## Build

This shell is not part of the default Godot build.

Build it from this directory with SCons:

```bash
cd engine/shells/sdl
scons -Q platform=windows target=debug -j4
```

The SDL shell currently defaults to `target=debug` for correctness while the
shell path is still being stabilized.

If you want to try a faster optimized build:

```bash
scons -Q platform=windows target=release -j4
```

For now, the SDL release build keeps the FireRed/game side on the stable
unoptimized codegen path to avoid known 64-bit optimization-sensitive
regressions in the transformed portable core, while still using the SDL shell's
release-facing build configuration.

Optional SCons variables:

- `sdl_include_dir=/path/to/include`
- `sdl_lib_dir=/path/to/SDL3/lib`
- `sdl_bin_dir=/path/to/SDL3/bin`

The default Windows paths assume an SDL3 install where headers live under
`.../include`, libraries live under `.../lib`, and the runtime DLL lives under
`.../bin`.

SDL-specific intermediates now live under `build/sdl/` so SDL and Godot builds do
not wipe each other's archives/objects.

## Run

```bash
build/decomp_engine_sdl.exe baserom.gba
```

Optional second argument sets the state file path.

## Debugging Notes

- Use this shell as the primary portable-runtime validation path.
- If a graphics bug appears only on portable builds, audit raw `INCBIN` users
  and portable embedded asset headers before changing unrelated assets.
- If a native crash survives lightweight tracing, capture a WinDbg backtrace
  early with `!analyze -v`, `.ecxr`, and `kv`.
