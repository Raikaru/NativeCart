# Runtime ROM mods (BPS) — SDL shell

The SDL shell loads a **clean FireRed ROM from disk**, optionally applies a **BPS patch entirely in memory**, then hands the resulting bytes to `engine_load_rom()`. The base ROM file is **never written**.

## Usage

```text
decomp_engine_sdl <rom.gba> [--bps <patch.bps>] [--mod-manifest <file>] [state_file]
```

- **`--bps`**: Path to a BPS1 patch. **Highest priority** when present (overrides manifest and env).
- **`--mod-manifest`**: Optional tiny text file; the first non-empty line that does not start with `#` is interpreted as a path to a `.bps` **relative to the manifest file’s directory**. Used only if `--bps` was not given.
- **`FIRERED_BPS_PATCH`**: If set and neither `--bps` nor a resolved manifest path is used, this environment variable supplies the default `.bps` path.
- **`state_file`**: Same as before (saved state path); optional, defaults to `sdl_shell.state`.

### Vanilla (no mod)

```text
decomp_engine_sdl firered.gba
```

### With BPS

```text
decomp_engine_sdl firered.gba --bps MyHack.bps
```

## Validation / errors

The BPS layer checks magic (`BPS1`), expected source size, source CRC (wrong clean ROM), patch structure, output size, and output CRC. Failures print a short message to stderr and exit before `engine_load_rom`.

If the patch file’s own CRC (footer) does not match, a **warning** is printed unless `FIRERED_BPS_IGNORE_PATCH_CRC` is set in the environment.

## Architecture (UPS later)

- **ROM load** (`sdl_shell_read_file`) → optional **BPS apply** (`mod_patch_bps_apply`) → **`engine_load_rom`** (backend copies ROM again internally).
- **`mod_patch_format.h`**: format enum for future backends.
- **UPS**: add e.g. `mod_patch_ups.c` with the same “malloc output, return error string” contract, then branch on extension or an explicit `--ups` flag in the shell without changing the engine ROM pipeline.

## Limits

- Single patch in this pass (no patch stacking or conflict detection).
- Output size capped (see `MOD_PATCH_MAX_OUTPUT` in `mod_patch_bps.c`).
