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
- `F11` = cycle **turbo tier** (1x → 2x → 4x → 8x → MAX → 1x)
- `Esc` = quit

## Turbo / speed-up

The SDL shell uses a **tiered turbo** mode (similar in spirit to console RPG fast-forward):

| Tier | Simulation per host spin | Present | Audio |
|------|--------------------------|---------|-------|
| 1x | exactly **1** frame | every spin | on (normal) |
| 2x | exactly **2** frames | every spin | on (per sim frame) |
| 4x | **time-budget** (~4.5 ms) + cap **8** frames | every 3rd spin | muted (SDL queue cleared; not audio-bound) |
| 8x | **time-budget** (~6 ms) + cap **16** frames | every 5th spin | muted |
| MAX | **short budget** (~2.2 ms) + cap **24** frames | every 8th spin | muted |

**4x / 8x / MAX** do not run huge fixed batches: each host iteration only simulates until a small wall-clock budget is used or a per-spin frame cap is hit, then returns so `SDL_PollEvent` and present logic run again soon.

- **Shift+Tab** also cycles the same tiers (and clears Tab-sprint).
- **Tab** (hold, without Shift): temporary **sprint** — uses the 8x policy whenever the selected tier is below 8x (shown as a `+` in the window title). At 8x or MAX, sprint does not change policy.

Each host iteration still runs `SDL_PollEvent` first, so keyboard/gamepad stay responsive. Only **1x** uses the 60 FPS delay cap; higher tiers do not sleep for pacing.

## Environment (optional)

- **`FIRERED_VERBOSE_SDL=1`** — print informational messages to stderr (renderer name, vsync interval, gamepad connect/disconnect, first audio-activity line, F5 export path). Errors (window/renderer/audio failures) are always printed.
- **`FIRERED_SDL_VSYNC`** — after creating the renderer, calls `SDL_SetRenderVSync`: unset or default → **1** (vsync on); **`0`** → off; **`adaptive`** or **`-1`** → `SDL_RENDERER_VSYNC_ADAPTIVE` when supported.
- **1x pacing:** if the driver reports **vsync active** (`SDL_GetRenderVSync` → non-zero interval), the shell **skips** the extra `SDL_Delay` frame budget so pacing does not stack vsync blocking with an artificial sleep.

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
