# ROM asset bind framework (portable)

`cores/firered/portable/firered_portable_rom_asset_bind.{h,c}` centralize the
**common flow** for ROM-backed asset families:

1. Minimum ROM size gate (`engine_memory_get_loaded_rom_size` vs `ENGINE_ROM_ADDR`).
2. Optional **env-gated** stderr trace (`trace_env_var` + `trace_prefix`).
3. Family **`try_env`** (typically: **env hex vars** first, then **`firered_portable_rom_asset_profile_lookup`**
   for built-in rows keyed by `firered_portable_rom_compat_get()` — CRC + `profile_id`) → on failure,
   optional **`clear_outputs`**.
4. **`firered_portable_rom_asset_want_signature_scan`**: per-family
   `use_compat_scan_gate` + optional `force_scan_env_var` (e.g. copyright vs title logo).
5. Optional **`trace_rom_too_small`** overrides the default too-small stderr line.
6. Family **`try_scan`** (signature / LZ logic lives in the family).
7. Optional trace lines for “skip scan” and “fallback to *_Portable”.

## API highlights

| Piece | Role |
|--------|------|
| `FireredRomAssetBindParams` | Describes one family’s bind attempt (callbacks + trace strings). |
| `firered_portable_rom_asset_bind_run` | Executes the flow; returns `TRUE` if `try_env` or `try_scan` bound. |
| `firered_portable_rom_asset_parse_hex_env` | Shared `strtoul(..., 0)` hex offset parsing. |
| `firered_portable_rom_asset_want_signature_scan` | Reusable compat / force-scan policy. |
| `trace_rom_too_small` (in params) | Optional override for ROM-too-small trace line. |

## What stays in the family

- Env **variable names** and validation.
- **Signatures**, search caps, LZ follow-up rules, multi-blob layout.
- Writing **`user`** output pointers.

## ROM asset profile layer (built-in offsets)

`cores/firered/portable/firered_portable_rom_asset_profile.{h,c}` holds a **small** static table:
match on optional **ROM CRC32** and/or **`profile_id`** from `firered_portable_rom_compat_get()`, then
return `pal` / `pal2` / `tiles` / `map` offsets (`FireredRomAssetProfileOffsets` + mask bits).

**Priority** (each binder): **environment** → **profile table** → **scan** → **`*_Portable`**.

The table ships with a **sentinel row** that never matches; add real rows next to the documented example
in the `.c` file. Not a catalog or download system.

## Current adoption

- **Oak speech main BG** — `firered_portable_oak_speech_bg_rom.c` uses the framework end-to-end;
  `try_env` uses env for tiles/map, optional profile **palette** when env omits `PAL_OFF`, or a full
  **profile** tiles+map row when env omits both.
- **Title copyright / PRESS START** — `firered_portable_title_screen_rom.c` (`title_copyright_*`
  callbacks + `firered_portable_rom_asset_bind_run`); same `FIRERED_TRACE_TITLE_ROM` as other
  title-rom traces, with `trace_prefix` `[firered title-rom copyright]`; **`try_env`** consults the
  profile table (`FIRERED_ROM_ASSET_FAMILY_COPYRIGHT_PRESS_START`) when env vars are unset.
- **Title game title logo** — same file (`title_logo_*`, **3 outputs**: pal + tiles + map);
  `try_env` also accepts a **full** profile row (`HAS_PAL` + `HAS_TILES` + `HAS_MAP`) when all three
  env vars are unset; `use_compat_scan_gate = FALSE` (always signature-scan after env/profile fails);
  optional `trace_rom_too_small` preserves the legacy `engine_memory_get_loaded_rom_size()` wording.
- **Main menu BG palettes** — `firered_portable_main_menu_pal_rom.c`: two **raw 32-byte**
  `graphics/main_menu/*.gbapal` blobs; env needs **both** offsets; **profile** can supply both
  (`HAS_PAL` + `HAS_PAL2`); scan finds **first** match for each stock prefix; `use_compat_scan_gate = TRUE` with
  `FIRERED_ROM_MAIN_MENU_PAL_SCAN` to force scan on vanilla-like profiles.

## Suggested migrations (later)

- **Other static text families** — use `firered_portable_rom_text_family` (`docs/portable_rom_text_family.md`);
  Oak intro is the reference consumer.

No global registry, no descriptor language — add a `FireredRomAssetBindParams` stack
object (or `static const` where callbacks allow) per public entry point.
