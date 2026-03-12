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
- `Z` = B
- `X` = A
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

Optional SCons variables:

- `sdl_include_dir=/path/to/include`
- `sdl_lib_dir=/path/to/SDL2/lib`

The default Windows paths assume an MSYS2-style SDL2 install where headers live
under `.../include/SDL2` and libraries live under `.../lib`.

## Run

```bash
build/decomp_engine_sdl.exe baserom.gba
```

Optional second argument sets the state file path.
