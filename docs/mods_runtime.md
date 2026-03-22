# Runtime ROM mods (BPS / UPS) — SDL shell

The SDL shell loads a **clean FireRed ROM from disk**, optionally applies a **BPS or UPS patch entirely in memory**, then hands the resulting bytes to `engine_load_rom()`. The base ROM file is **never written**.

## Usage

```text
decomp_engine_sdl <rom.gba> [--bps <patch.bps> | --ups <patch.ups>] [--mod-manifest <file>] [state_file]
```

Do **not** pass both `--bps` and `--ups`.

- **`--bps`**: Path to a BPS1 patch. Highest priority when present (overrides manifest and env).
- **`--ups`**: Path to a UPS1 patch. Same priority as `--bps`; mutually exclusive with `--bps`.
- **`--mod-manifest`**: Optional tiny text file; the first non-empty line that does not start with `#` is a path to a **`.bps` or `.ups`** file **relative to the manifest file’s directory**. Used only if neither `--bps` nor `--ups` was given. The extension selects the backend.
- **`FIRERED_MOD_PATCH`**: If set and no argv patch / manifest patch is used, this path is used; extension must be `.bps` or `.ups` (case-insensitive).
- **`FIRERED_BPS_PATCH`**: If still no patch path, this supplies a **BPS-only** default (backward compatible).
- **`state_file`**: Saved state path; optional, defaults to `sdl_shell.state`.

### Vanilla (no mod)

```text
decomp_engine_sdl firered.gba
```

### With BPS

```text
decomp_engine_sdl firered.gba --bps MyHack.bps
```

### With UPS

```text
decomp_engine_sdl firered.gba --ups MyHack.ups
```

## Validation / errors

### BPS

Magic (`BPS1`), expected source size, source CRC, output size/CRC, patch structure. Patch-file CRC mismatch is a **warning** unless `FIRERED_BPS_IGNORE_PATCH_CRC` is set.

### UPS

Implementation follows **Alcaro Flips `libups`** semantics:

- Magic **`UPS1`**
- **Variable-length integers** in the patch are **not** the same encoding as BPS; do not reuse BPS decoders.
- **Source size**: must match the patch after optional **reverse** handling (same patch can apply in the “forward” or “backward” direction when input/output sizes differ — see Flips).
- **CRC32** footer: validates input ROM, output ROM, and patch integrity. For equal-sized input/output, Flips accepts **either** ordering of the two ROM CRCs (bidirectional patch). Mismatch → `UPS: CRC mismatch`.
- **Patch file CRC**: compared to bytes **excluding the last 4** of the file. Mismatch prints a **warning** unless `FIRERED_UPS_IGNORE_PATCH_CRC=1` (same idea as BPS).

Malformed hunks, truncated XOR blocks, or junk before the footer → clear `UPS: ...` errors.

### Limits

- Single patch (no stacking).
- Output size capped at **128 MiB** (`MOD_PATCH_MAX_OUTPUT` in `mod_patch_bps.c` / `mod_patch_ups.c`).

## Architecture

- **ROM load** (`sdl_shell_read_file`) → optional **patch apply** (`mod_patch_bps_apply` or `mod_patch_ups_apply`) → **`engine_load_rom`**.
- **`mod_patch_format.h`**: `ModPatchFormat` enum (`BPS`, `UPS`).
- Backends: `engine/core/mod_patch_bps.c`, `engine/core/mod_patch_ups.c`.

## Launcher

The SDL launcher scans **`mods/`** for **`*.bps`** and **`*.ups`**, and passes **`--bps`** or **`--ups`** automatically from the file extension. Only one enabled mod is applied per launch.

## IPS / xdelta

Not implemented in this pipeline (use external tools or a future backend).

## Portable SDL build vs ROM-backed hacks

Patching **does** replace the bytes mapped at `ENGINE_ROM_ADDR` (see `engine_memory_init`). Many hacks still **look vanilla** because the **SDL portable** target does **not** draw most UI from that ROM image:

- **`PORTABLE_FAKE_INCBIN`**: `INCBIN` becomes `{0}` in `include/global.h` — graphics that retail reads from ROM are **not** embedded that way in portable builds.
- **`src_transformed/*_portable_assets.h` + `#define gFoo gFoo_Portable`**: e.g. `title_screen.c` under `#ifdef PORTABLE` forces most title assets from **compile-time byte arrays**. **Exception (ROM-backed):** the **game title logo** (BG0 pal + LZ tiles + LZ map) is resolved from the **mapped ROM** when `firered_portable_title_screen_try_bind_game_title_logo()` succeeds (see `docs/rom_backed_runtime.md` §5); other title layers remain `_Portable`.
- **`cores/firered/generated/data/*_portable_data.c`** and **`firered_portable_resolve_script_ptr`**: many scripts resolve through **token → static pointer tables** (`gFireredPortableEventScriptPtrs`, …) built from the **decomp baseline**, not from hack ROM streams.
- **`rom_header_portable.c`**: after `engine_memory_init`, **`firered_portable_sync_rom_header_from_cartridge()`** copies **game code** (`0xAC`) and **software version** (`0xBC`) from mapped ROM into `RomHeaderGameCode` / `RomHeaderSoftwareVersion` (see `docs/rom_backed_runtime.md`). Other header fields may still be compile-time or unused on portable.

So: **BPS/UPS apply correctly**, but **visible content** is often **wired to vanilla compiled data**. Large hacks (e.g. Unbound: 32 MiB, different engine/layout) are **far beyond** what this portable data model assumes.

### Narrow ROM consumption trace

After mapping ROM, one stderr line can show the **actual** GBA header in mapped memory (changes with hacks):

```text
set FIRERED_TRACE_ROM_CONSUMPTION=1
```

Look for `[firered rom-trace] GBA header in mapped ROM: ...` — if game code/title bytes match the **patched** file but the UI still looks stock, the bypass is **downstream** (portable assets / script tables), not the mod loader.
