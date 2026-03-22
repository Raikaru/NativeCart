# SDL Shell

This directory contains the **canonical** SDL/native shell for the generic
`engine/core/` + `cores/firered/` runtime path — the **only** supported portable host.

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

SDL-specific intermediates live under `build/sdl/` (separate from the top-level
`build/` outputs such as `runtime_progress_runner`).

## Run

```bash
build/decomp_engine_sdl.exe baserom.gba
```

Optional second argument sets the state file path.

## In-game save file (battery flash image)

Portable builds persist the 128 KiB flash image as `firered_save.sav`.

- **Default location:** per-user app data under a neutral folder `FireRedPortable`,
  e.g. on Windows `%APPDATA%\FireRedPortable\firered_save.sav`.
- **Older saves** outside `FireRedPortable` can be used by setting
  `FIRERED_PORTABLE_FLASH_PATH`, or by copying/renaming the `.sav` into the
  default folder.
- **Overrides (optional):**
  - `FIRERED_PORTABLE_FLASH_PATH` — full path to the `.sav` file.
  - `FIRERED_SAVE_DIR` — directory containing `firered_save.sav`.

## Debugging Notes

- Use this shell as the primary portable-runtime validation path.
- If a graphics bug appears only on portable builds, audit raw `INCBIN` users
  and portable embedded asset headers before changing unrelated assets.
- If a native crash survives lightweight tracing, capture a WinDbg backtrace
  early with `!analyze -v`, `.ecxr`, and `kv`.
