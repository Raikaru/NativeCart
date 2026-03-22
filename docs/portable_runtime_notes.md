# Portable Runtime Notes

This note captures the highest-value debugging patterns from the current SDL and
portable FireRed stabilization work.

## Primary Validation Path

Use the SDL debug shell as the default verification path for portable runtime
and UI fixes.

Windows SDL debug build:

```powershell
cd engine\shells\sdl
scons -Q platform=windows target=debug -j4 `
  sdl_include_dir="C:\path\to\SDL3\x86_64-w64-mingw32\include" `
  sdl_lib_dir="C:\path\to\SDL3\x86_64-w64-mingw32\lib" `
  sdl_bin_dir="C:\path\to\SDL3\x86_64-w64-mingw32\bin"
```

Run:

```powershell
build\decomp_engine_sdl.exe pokefirered.gba
```

SDL debug should be the first stop for correctness, graphics corruption, and
native crash reproduction.

### In-game save path (`agb_flash.c`)

Battery-backed flash is stored as `firered_save.sav`. Defaults use a neutral
per-user folder `FireRedPortable` (see `src_transformed/agb_flash.c`). There is
no automatic read from legacy editor userdata layouts; point
`FIRERED_PORTABLE_FLASH_PATH` at an existing file if you need it.

Optional environment variables: `FIRERED_PORTABLE_FLASH_PATH` (full path to the
`.sav` file), `FIRERED_SAVE_DIR` (directory for `firered_save.sav`).

### Main menu seam (compiled portable path)

The **main menu is not read from the loaded ROM**: it is implemented in
`src_transformed/main_menu.c` with vanilla FireRed’s three modes only (NEW GAME /
CONTINUE / MYSTERY GIFT). Patched ROM bytes do not replace this C path.

Optional narrow trace (SDL / debug builds):

```powershell
$env:FIRERED_TRACE_MAIN_MENU = "1"
.\build\decomp_engine_sdl.exe pokefirered.gba
```

Logs menu mode, save status, and a **ROM identity** line (CRC-32 of the full loaded image + game code / version) from `firered_portable_rom_queries`; see `docs/portable_rom_queries.md`.

**ROM hack compatibility profile** (kind / flags / small known-profile table) is built after each load in `firered_portable_rom_compat`; see `docs/portable_rom_compat.md`. Optional trace: `FIRERED_TRACE_ROM_COMPAT=1`.

## Portable Graphics Failure Modes

Two graphics problems account for most portable UI corruption:

1. Raw `INCBIN` assets silently disappearing under portable builds.
2. Portable embedded asset headers drifting from canonical `graphics/` files.

### Raw `INCBIN` gotcha

Under `PORTABLE_FAKE_INCBIN`, raw `INCBIN_U8` / `INCBIN_U32` definitions can
collapse to fake placeholder data instead of real graphics bytes. When a screen
looks like repeated garbage tiles, blank regions, or partially missing UI, audit
raw `INCBIN` users first.

Common symptom families:

- party menu slot tilemaps missing or repeating
- Pokemon Storage backgrounds or tabs drawing black or scrambled
- tiny "blank" helper tiles showing garbage in battle UI corners
- icon sheets or cursor assets appearing as all-zero/fake data

Fix pattern:

1. Find the runtime global being used by the broken screen.
2. Check whether it still comes from raw `INCBIN`.
3. On `PORTABLE`, replace it with a byte-accurate embedded copy of the
   canonical asset.
4. Rebuild and re-test before touching unrelated assets.

### Embedded asset drift

Some portable headers embed graphics or tilemaps directly in C arrays. Those
arrays can drift from the canonical files under `graphics/`.

Fix pattern:

1. Compare the portable array bytes to the canonical asset bytes.
2. Only patch arrays that are proven mismatches.
3. Keep the canonical `graphics/` file as the source of truth.

Do not swap assets speculatively. Byte-verify first.

## Portable Text Failure Modes

Most recent text bugs came from mixing plain C or UTF-8 strings with FireRed's
GBA byte-string format.

High-probability symptoms:

- accented text like `POKe BALL` or `POK?MON`
- placeholder text like `STR_VAR_1` showing literally
- bag labels, move menu labels, or town names rendering differently from battle
- width/placement bugs where text merges together or centers incorrectly

Fix pattern:

1. Audit the exact call path that builds the broken string.
2. Identify whether that path is using:
   - raw FireRed byte strings
   - plain C strings
   - UTF-8 C strings
   - mixed control/template strings
3. Normalize at the narrowest shared point that truly owns the conversion.
4. Preserve native FireRed byte strings as-is.

Do not assume a rendering bug until the string source and conversion layer are
checked.

### Generated portable text data

If descriptions or multi-line UI text are merged together, inspect generated
portable data before changing text printers.

Known example:

- item descriptions generated from `src/data/items.json.txt`
- emitted portable data in
  `pokefirered_core/generated/src/data/items_portable_data.c`
- generator logic in `tools/portable_generators/generate_portable_item_data.py`

Escapes like `\n` must remain escaped in generated C strings. If the generator
turns them into literal source newlines too early, the output data is already
wrong before rendering begins.

## Pokemon Storage Crash Pattern

Pokemon Storage has a recurring stale-sprite-pointer failure mode.

Typical shape:

- a task caches a sprite pointer
- the screen transitions or tears down part of the UI
- the underlying `gSprites` slot is destroyed or reused
- later code reuses the stale pointer and crashes

Affected families seen so far:

- item icon sprites
- display panel sprites
- party/box transition sprites
- marking combo sprites

Fix pattern:

1. Treat cached sprite pointers as invalid after transitions unless proven live.
2. Before reuse, verify the pointer still refers to a live `gSprites` entry.
3. If not live, null it out and skip work instead of dereferencing it.

For Pokemon Storage bugs, separate two questions early:

1. Is this broken because an asset is fake/missing on `PORTABLE`?
2. Is this broken because runtime sprite state went stale across a task/menu
   transition?

## Native Crash Workflow

If targeted tracing does not expose the fault quickly, use WinDbg earlier
instead of iterating on more speculative patches.

Minimal Windows flow:

1. Launch `build\decomp_engine_sdl.exe` under WinDbg.
2. Reproduce the crash.
3. Run:

```text
!analyze -v
.ecxr
kv
```

The top frames from `kv` are usually enough to identify the exact portable seam.

## SDL3 Status

The SDL shell is already migrated to SDL3. Keep shell-side debugging and build
notes aligned with SDL3 paths, APIs, and local dependency locations.
