# ROM-backed text family helper (portable)

`cores/firered/portable/firered_portable_rom_text_family.{h,c}` centralize the **common flow**
for bounded **GBA-encoded** (EOS-terminated) string families loaded from the mapped ROM:

1. Optional per-entry **hex env** offset (`firered_portable_rom_asset_parse_hex_env`).
2. Optional **`try_profile_rom_off`** callback (fork can wire `firered_portable_rom_asset_profile` or a local table).
3. Optional **`try_scan`** callback; if NULL, **first** full-ROM **memcmp** vs the compiled fallback bytes.
4. **`get_fallback`** compiled string.

Families supply `FireredRomTextFamilyParams` (entry count, env key table, min scan length, trace env/prefix)
and call `firered_portable_rom_text_family_bind_all` once into a `const u8 *` cache.

## Adoption

- **Oak intro** — `firered_portable_oak_speech_text_rom.c` (`FIRERED_TRACE_OAK_SPEECH_ROM`,
  `scan_min_match_len = 12`, no custom scan, no profile callback yet).
- **Controls guide + Pikachu intro pages** (before Oak dialogue) —
  `firered_portable_early_new_game_help_text_rom.c` (`FIRERED_TRACE_EARLY_NEW_GAME_HELP_ROM`,
  env keys `FIRERED_ROM_CTRL_GUIDE_TX_*`, `FIRERED_ROM_PIKACHU_INTRO_TX_Page{1,2,3}`).
- **Optional extra intro prose pages** (prepend before the controls guide) — **not** a `rom_text_family` slice:
  `firered_portable_new_game_intro_prose_rom.c` uses a contiguous ROM **`u32`** pointer table + count
  (`FIRERED_ROM_NEW_GAME_INTRO_PROSE_PTR_TABLE_OFF`, `FIRERED_ROM_NEW_GAME_INTRO_PROSE_PAGE_COUNT`); see **`docs/rom_backed_runtime.md`** §8c.
- **Region map mapsec display names** — `firered_portable_rom_region_map_mapsec_names.c`
  (`REGION_MAP_MAPSEC_NAME_ENTRY_COUNT` entries, `scan_min_match_len = 6`, **no** `env_key_names`).
  Refresh tries a **`u32[]` pointer-table** bind (**`FIRERED_ROM_REGION_MAP_MAPSEC_NAME_PTR_TABLE_OFF`**
  / `FireredRomU32TableProfileRow`) first; then **`try_profile_rom_off`** sparse string rows;
  then default scan vs **`gRegionMapMapsecNames_Compiled`**.

## Adding a family

1. Define an entry index enum and compiled `extern const u8` fallbacks (or `get_fallback` switch).
2. Static `const char *const env_key_names[]` (NULL slots allowed).
3. Fill `FireredRomTextFamilyParams`; implement optional `try_profile_rom_off` / `try_scan`.
4. On first use, `bind_all` into a static cache; expose `get_text(id)` reading `cache[id]`.

Keep tables small; no global string registry in the helper.
