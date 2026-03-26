# Runtime ROM mods (BPS / UPS) — SDL shell

The SDL shell loads a **clean FireRed ROM from disk**, optionally applies a **BPS or UPS patch entirely in memory**, then hands the resulting bytes to `engine_load_rom()`. The base ROM file is **never written**.

## Usage

```text
decomp_engine_sdl <rom.gba> [--bps <patch.bps> | --ups <patch.ups>] [--require-mod] [--strict-runtime] [--mod-manifest <file>] [state_file]
```

Do **not** pass both `--bps` and `--ups`.

- **`--bps`**: Path to a BPS1 patch. Highest priority when present (overrides manifest and env).
- **`--ups`**: Path to a UPS1 patch. Same priority as `--bps`; mutually exclusive with `--bps`.
- **`--require-mod`**: Refuse to boot vanilla if no patch is actually resolved. Use this for launcher-driven mod sessions where silent fallback is misleading.
- **`--strict-runtime`**: Enable provenance-aware strict checks for mod sessions. This is **not** "crash on any fallback"; strict failure only happens when a seam appears mod-changed and still fell back to compiled data.
- **`--mod-manifest`**: Optional tiny text file; the first non-empty line that does not start with `#` is a path to a **`.bps` or `.ups`** file **relative to the manifest file’s directory**. Used only if neither `--bps` nor `--ups` was given. The extension selects the backend.
- **`FIRERED_MOD_PATCH`**: If set and no argv patch / manifest patch is used, this path is used; extension must be `.bps` or `.ups` (case-insensitive).
- **`FIRERED_BPS_PATCH`**: If still no patch path, this supplies a **BPS-only** default (backward compatible).
- **`state_file`**: Saved state path; optional, defaults to `sdl_shell.state`.

### Provenance-aware strict mode (current seams)

Runtime strictness currently enforces two seam families:

- **experience tables** (`FIRERED_ROM_EXPERIENCE_TABLES_OFF`)
- **title-core** components:
  - `game_title_logo`
  - `copyright_press_start`

Per launch, that seam is classified as:

- `UNCHANGED`: ROM candidate matches compiled baseline (`gExperienceTables`) or no usable ROM candidate was resolved.
- `CHANGED_AND_BOUND`: candidate differs from baseline and was accepted as active ROM-backed data.
- `CHANGED_BUT_FELL_BACK`: candidate differs from baseline but failed seam validation, so runtime kept compiled fallback.

Strict mod runtime hard-fails **only** for `CHANGED_BUT_FELL_BACK`.
For title-core failures, the error line names the exact component:

- `Strict mod runtime provenance failure: seam=title_core component=game_title_logo state=CHANGED_BUT_FELL_BACK`
- `Strict mod runtime provenance failure: seam=title_core component=copyright_press_start state=CHANGED_BUT_FELL_BACK`

Scope note: this strict check covers the title-core seam only. Other title-scene assets (box-art, border/background palette, slash/flames, etc.) remain outside strict provenance enforcement in this pass.

Strict runtime is gated by two runtime signals:

- `FIRERED_RUNTIME_MOD_REQUESTED=1` (set by SDL shell when `--require-mod` is used)
- `FIRERED_RUNTIME_STRICT_MOD=1` (set by SDL shell when `--strict-runtime` is used, or when `FIRERED_STRICT_MOD_RUNTIME=1` is present)

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

The SDL launcher scans **`mods/`** for **`*.bps`** and **`*.ups`**, passes **`--bps`** or **`--ups`** automatically from the file extension, and also adds **`--require-mod`** so failed mod resolution does not silently test vanilla. Only one enabled mod is applied per launch.

## IPS / xdelta

Not implemented in this pipeline (use external tools or a future backend).

## Portable SDL build vs ROM-backed hacks

Patching **does** replace the bytes mapped at `ENGINE_ROM_ADDR` (see `engine_memory_init`). Many hacks still **look vanilla** because the **SDL portable** target does **not** draw most UI from that ROM image:

- **`PORTABLE_FAKE_INCBIN`**: `INCBIN` becomes `{0}` in `include/global.h` — graphics that retail reads from ROM are **not** embedded that way in portable builds.
- **`src_transformed/*_portable_assets.h` + `#define gFoo gFoo_Portable`**: title assets still have compiled fallbacks, but runtime now attempts ROM binding first for both static and effect-heavy visible title layers: **game title logo**, **copyright/PRESS START strip**, **box-art mon panel**, **background palette block**, **border BG**, and FIRERED effect assets (**flames pal/gfx**, **blank flames gfx**, **slash gfx**). A few utility sprite inputs (for example blank sprite tiles / slash pal source) still remain compile-backed in this pass.
- **`cores/firered/generated/data/*_portable_data.c`** and **`firered_portable_resolve_script_ptr`**: many scripts resolve through **token → static pointer tables** (`gFireredPortableEventScriptPtrs`, …) built from the **decomp baseline**, not from hack ROM streams.
- **`rom_header_portable.c`**: after `engine_memory_init`, **`firered_portable_sync_rom_header_from_cartridge()`** copies **game code** (`0xAC`) and **software version** (`0xBC`) from mapped ROM into `RomHeaderGameCode` / `RomHeaderSoftwareVersion` (see `docs/rom_backed_runtime.md`). Other header fields may still be compile-time or unused on portable.

So: **BPS/UPS apply correctly**, but **visible content** is often **wired to vanilla compiled data**. Large hacks (e.g. Unbound: 32 MiB, different engine/layout) are **far beyond** what this portable data model assumes.

### Narrow ROM consumption trace

After mapping ROM, one stderr line can show the **actual** GBA header in mapped memory (changes with hacks):

```text
set FIRERED_TRACE_ROM_CONSUMPTION=1
```

Look for `[firered rom-trace] GBA header in mapped ROM: ...` — if game code/title bytes match the **patched** file but the UI still looks stock, the bypass is **downstream** (portable assets / script tables), not the mod loader.
