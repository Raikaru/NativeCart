# ROM compatibility architecture (consolidation snapshot)

This document **freezes the current layered model** after resolver overlays for **all six portable token bases (`0x80000000` … `0x85000000`)**, for consolidation and **choosing the next slice** without ad‑hoc one‑hack work.

## 1. Load path (patched ROM → mapped memory)

| Step | Where | Notes |
|------|--------|--------|
| Read file | `engine/shells/sdl/main.c` | `sdl_shell_read_file` |
| Optional BPS/UPS | `mod_patch_*.c` | In‑memory patch; **no disk write** of patched ROM |
| Handoff | `engine_load_rom` → `core->load_rom` | **`engine/shells/sdl/main.c`** logs CRC + size when **`FIRERED_TRACE_MOD_LAUNCH=1`** |
| Copy + map | `engine_runtime_backend.c` → `engine_memory_init` | ROM copied; **`ENGINE_ROM_ADDR`** view is the **final** bytes (vanilla or patched) |
| Portable hooks | `#ifdef PORTABLE` in `engine_backend_init` | **`firered_portable_sync_rom_header_from_cartridge`**, **`firered_portable_rom_queries_invalidate_cache`**, **`firered_portable_rom_compat_refresh_after_rom_load`**, ROM-backed tables (dex, cries, HM, Deoxys, experience, type chart, species info, **mon pic layout pack**, **TM/HM learnsets**, **level-up learnsets pack**, **egg moves pack**, **evolution table**, **battle moves**, **move names pack**, **Battle Tower mon templates pack**, **battle terrain pack**, **tutor tables**, **heal locations**, region map section grids / layout / fly destinations / **mapsec display names**, **wild encounter family pack**, **map layout metatiles**, **map header scalars**, …), **`firered_portable_init_map_object_event_script_words`** |

**Validation:** With **`FIRERED_TRACE_MOD_LAUNCH=1`**, stderr shows **`calling engine_load_rom`** / **`engine_load_rom OK`** and **`crc32_final`** for the buffer actually handed to the core — **including post‑patch** digests when `--bps` / `--ups` is used.

**Offline slice checks:** For a candidate **`FIRERED_ROM_*`** offset and length, use **`tools/rom_blob_inspect.py`** and the workflow in **`docs/rom_blob_inspection_playbook.md`** (hex dump, LE word stats, stride / pointer sanity).

**Map-generated data wall (design / inventory only):** **`docs/portable_rom_map_generated_data_playbook.md`**; quick scale pass **`python tools/inventory_map_generated_data.py`** (see also **`docs/generated_data_rom_seam_playbook.md`**).

## 2. Priority rules (aligned across subsystems)

| Subsystem | Order | Implementation |
|-----------|--------|------------------|
| **ROM `u32[]` tables** (std scripts, special vars, field effects `FLDEFF_*`, battle AI scripts/fragments, battle anims general, **resolver overlays `0x80`–`0x85`**) | **Env hex** → **profile rows** (`rom_crc32` / `profile_id`) → *(no table)* | `firered_portable_rom_u32_table_resolve_base_off` |
| **ROM species→National `u16[]`** (same count as decomp) | **Env hex** → **profile rows** → compiled `sSpeciesToNationalPokedexNum` | `firered_portable_rom_species_national_dex.c` (offset via same env/profile pattern as `u32[]` helpers) |
| **ROM Hoenn dex `u16[]`** (species→Hoenn #, Hoenn order→National #) | **Two optional env/profile offsets** → compiled | `firered_portable_rom_species_hoenn_dex_tables.c` |
| **ROM species→cry ID `u16[]`** (post–OLD_UNOWN species, `SpeciesToCryId`) | **`FIRERED_ROM_SPECIES_CRY_ID_TABLE_OFF`** → compiled `cry_ids.h` | `firered_portable_rom_species_cry_id_table.c` |
| **ROM HM move list `u16[]`** (`IsHMMove2`, 9 words + `0xFFFF` sentinel) | **`FIRERED_ROM_HM_MOVES_TABLE_OFF`** → compiled `sHMMoves` | `firered_portable_rom_hm_moves_table.c` |
| **ROM Deoxys base stats `u16[]`** (`GetDeoxysStat`, 6 words) | **`FIRERED_ROM_DEOXYS_BASE_STATS_TABLE_OFF`** → compiled `sDeoxysBaseStats` | `firered_portable_rom_deoxys_base_stats_table.c` |
| **ROM experience tables `u32[]`** (`ExperienceTableGet`) | **`FIRERED_ROM_EXPERIENCE_TABLES_OFF`** → compiled `gExperienceTables` | `firered_portable_rom_experience_tables.c` |
| **ROM type chart `u8[336]`** (`TYPE_EFFECT_*` / `gTypeEffectivenessActivePtr`) | **`FIRERED_ROM_TYPE_EFFECTIVENESS_OFF`** → compiled `gTypeEffectiveness` | `firered_portable_rom_type_effectiveness_table.c` |
| **ROM species info `struct` table** (`gSpeciesInfo` macro / `gSpeciesInfoActive`) | **`FIRERED_ROM_SPECIES_INFO_TABLE_OFF`** → compiled **`gSpeciesInfo_Compiled`** | `firered_portable_rom_species_info_table.c` |
| **ROM mon pic layout pack** (`gMonFrontPicCoords` / `gMonBackPicCoords` / `gEnemyMonElevation` macros) | **`FIRERED_ROM_MON_PIC_LAYOUT_PACK_OFF`** → compiled **`gMonFrontPicCoords_Compiled` / `gMonBackPicCoords_Compiled` / `gEnemyMonElevation_Compiled`** | `firered_portable_rom_mon_pic_layout.c` |
| **ROM TM/HM learnset `u32[2]` table** (`gTMHMLearnsetsActive` / `sTMHMLearnsets` macro) | **`FIRERED_ROM_TMHM_LEARNSETS_TABLE_OFF`** → compiled **`sTMHMLearnsets_Compiled`** | `firered_portable_rom_tmhm_learnsets_table.c` |
| **ROM egg moves `u16` stream** (`FireredEggMovesTable` / `gEggMoves`) | **`FIRERED_ROM_EGG_MOVES_PACK_OFF`** → compiled **`gEggMoves`** | `firered_portable_rom_egg_moves_table.c` |
| **ROM level-up learnsets** (`FireredLevelUpLearnsetsTable` / **`gLevelUpLearnsets_Compiled`**) | **`FIRERED_ROM_LEVEL_UP_LEARNSETS_PACK_OFF`** → compiled pointer table + lists | `firered_portable_rom_level_up_learnsets_family.c` |
| **ROM evolution table** (`gEvolutionTableActive` / `gEvolutionTable` macro) | **`FIRERED_ROM_EVOLUTION_TABLE_OFF`** → compiled **`gEvolutionTable_Compiled`** | `firered_portable_rom_evolution_table.c` |
| **ROM battle moves** (`gBattleMovesActive` / `gBattleMoves` macro) | **`FIRERED_ROM_BATTLE_MOVES_TABLE_OFF`** → compiled **`gBattleMoves_Compiled`** | `firered_portable_rom_battle_moves_table.c` |
| **ROM move names** (`FireredMoveNamesBytes` / **`gMoveNames_Compiled`**) | **`FIRERED_ROM_MOVE_NAMES_PACK_OFF`** → compiled **`gMoveNames_Compiled`** | `firered_portable_rom_move_names_table.c` |
| **ROM Battle Tower mon templates** (`gBattleTowerLevel50MonsActive` / **`gBattleTowerLevel50Mons_Compiled`**, level-100 pair) | **`FIRERED_ROM_BATTLE_TOWER_MON_TEMPLATES_PACK_OFF`** → compiled **`gBattleTowerLevel50Mons_Compiled` / `gBattleTowerLevel100Mons_Compiled`** | `firered_portable_rom_battle_tower_mon_templates.c` |
| **ROM move tutor** (`gTutorMovesActive` / `gTutorLearnsetsActive`, party menu macros) | **`FIRERED_ROM_TUTOR_MOVES_TABLE_OFF`** + **`FIRERED_ROM_TUTOR_LEARNSETS_TABLE_OFF`** → compiled **`sTutorMoves_Compiled` / `sTutorLearnsets_Compiled`** | `firered_portable_rom_tutor_tables.c` |
| **ROM heal / fly locations** (`gHealLocationsActive`, heal_location.c macro) | **`FIRERED_ROM_HEAL_LOCATIONS_TABLE_OFF`** → compiled **`sHealLocations_Compiled`** | `firered_portable_rom_heal_locations_table.c` |
| **ROM whiteout heal-center map idxs + healer NPC ids** (`gWhiteoutRespawnHealCenterMapIdxsActive` / `gWhiteoutRespawnHealerNpcIdsActive`, paired) | **`FIRERED_ROM_WHITEOUT_HEAL_CENTER_MAP_IDXS_OFF`** + **`FIRERED_ROM_WHITEOUT_HEALER_NPC_IDS_OFF`** → compiled **`sWhiteoutRespawnHealCenterMapIdxs_Compiled`** / **`sWhiteoutRespawnHealerNpcIds_Compiled`** | `firered_portable_rom_heal_locations_table.c` |
| **ROM region map section grids** (four `sRegionMapSections_*` tensors, single pack) | **`FIRERED_ROM_REGION_MAP_SECTION_GRIDS_PACK_OFF`** → compiled **`sRegionMapSections_*_Compiled`** | `firered_portable_rom_region_map_section_grids.c` |
| **ROM region map section layout** (corners + dimensions, paired) | **`FIRERED_ROM_REGION_MAP_SECTION_LAYOUT_PACK_OFF`** → compiled **`sMapSectionTopLeftCorners_Compiled` / `sMapSectionDimensions_Compiled`** | `firered_portable_rom_region_map_section_layout.c` |
| **ROM region map fly destinations** (`gMapFlyDestinationsActive`) | **`FIRERED_ROM_REGION_MAP_FLY_DESTINATIONS_TABLE_OFF`** → compiled **`sMapFlyDestinations_Compiled`** | `firered_portable_rom_region_map_fly_destinations.c` |
| **ROM region map mapsec display names** (`gRegionMapMapsecNamesResolved`, `GetMapName`) | **`FIRERED_ROM_REGION_MAP_MAPSEC_NAME_PTR_TABLE_OFF`** / **`FireredRomU32TableProfileRow`** → LE **`u32[]`** of string addresses; else **sparse** direct string profile rows → text-family **scan** vs **`gRegionMapMapsecNames_Compiled`** | `firered_portable_rom_region_map_mapsec_names.c` |
| **ROM assets** (title logo, copyright strip, Oak BG, main menu pals, LZ helpers) | **Env** → **asset profile** (`firered_portable_rom_asset_profile.c`) → **scan** (when allowed) → **`*_Portable`** | Per‑family binders; see `docs/portable_rom_asset_bind.md` |
| **Text families** (e.g. Oak speech) | **Per‑string env** → profile *(where wired)* → **scan vs compiled** → compiled | `firered_portable_rom_text_family.c` |
| **Compat classification** | Feeds **profile** matching + scan policy | `firered_portable_rom_compat.c` |

**Fallback:** Every ROM path above **falls back to compiled / `_Portable` / decomp snapshot** when env/profile/scan/ROM read fails — **vanilla with no env** should match **pre‑ROM‑migration** behavior for that vertical.

## 3. Compatibility matrix (what is generic now)

| Area | Consumes loaded ROM? | Default when unset |
|------|----------------------|---------------------|
| Cartridge header mirrors (`RomHeaderGameCode`, etc.) | Yes (sync after load) | Same as retail for vanilla |
| Title game logo (BG0) | Yes (env / scan / profile) | `_Portable` |
| Copyright / press start strip | Yes (env / scan / profile) | `_Portable` |
| Oak intro text | Yes (env / scan) | Compiled strings |
| Oak speech main BG | Yes (env / scan / profile) | `_Portable` |
| Main menu palettes | Yes (env / scan) | `_Portable` |
| `gStdScripts` / `gSpecialVars` / `gFieldEffectScriptPointers` (call sites) | Optional ROM `u32[]` | Compiled |
| `gBattleAI_ScriptsTable` / `gBattleAnims_General` | Optional ROM `u32[]` | Compiled |
| **`firered_portable_resolve_script_ptr` `0x81000000`** | Optional overlay (`i ≥ 213`) | Compiled table |
| **`firered_portable_resolve_script_ptr` `0x83000000`** | Optional full parallel table | Compiled + `0x83` fixup |
| **`firered_portable_resolve_script_ptr` `0x85000000`** | Optional overlay (all indices) | Compiled table |
| **`0x80000000` battle scripts** | Optional overlay (ROM `u32` only if **`0x08……`–`0x09……`**) | Compiled |
| **`0x82000000` field-effect tokens in bytecode** | Optional overlay (**`FIRERED_ROM_FIELD_EFFECT_PTRS_OFF`**, ROM `u32` only if **`0x08……`–`0x09……`**) | Compiled *(separate env from `FieldEffectStart` `FIRERED_ROM_FIELD_EFFECT_SCRIPT_PTRS_OFF`)* |
| **`0x84000000` battle anims** | Optional overlay (**`FIRERED_ROM_BATTLE_ANIM_PTRS_OFF`**, ROM `u32` only if **`0x08……`–`0x09……`**) | Compiled *(separate from `LaunchBattleAnimation` `FIRERED_ROM_BATTLE_ANIMS_GENERAL_PTRS_OFF`)* |
| **Species → National Dex / inverse** (`SpeciesToNationalPokedexNum`, `NationalPokedexNumToSpecies`) | Optional ROM **`u16[]`** (**`FIRERED_ROM_SPECIES_TO_NATIONAL_DEX_OFF`**, profile) — **structural** blob, same length as decomp | Compiled `sSpeciesToNationalPokedexNum` |
| **Hoenn ↔ National dex helpers** | Optional **two** ROM **`u16[]`** | Compiled Hoenn tables |
| **`SpeciesToCryId`** (post–OLD_UNOWN) | Optional ROM **`u16[]`** (**`FIRERED_ROM_SPECIES_CRY_ID_TABLE_OFF`**) | Compiled **`cry_ids.h`** |
| **`IsHMMove2`** | Optional ROM **9 × `u16`** (**`FIRERED_ROM_HM_MOVES_TABLE_OFF`**) | Compiled **`sHMMoves`** |
| **`ExperienceTableGet` / level & EXP UI** | Optional ROM **6 × (`MAX_LEVEL`+1) LE `u32`** (**`FIRERED_ROM_EXPERIENCE_TABLES_OFF`**) | Compiled **`gExperienceTables`** |
| **`TYPE_EFFECT_*` / type damage calc** | Optional ROM **336-byte** chart (**`FIRERED_ROM_TYPE_EFFECTIVENESS_OFF`**) | Compiled **`gTypeEffectiveness`** |
| **`gSpeciesInfo` / stats, types, growth, items** | Optional ROM **`NUM_SPECIES × 26`** bytes (**`FIRERED_ROM_SPECIES_INFO_TABLE_OFF`**) | Compiled **`gSpeciesInfo_Compiled`** |
| **`gBattleMoves` / move stats, battle scripts, PP** | Optional ROM **`MOVES_COUNT × 9`** bytes (**`FIRERED_ROM_BATTLE_MOVES_TABLE_OFF`**) | Compiled **`gBattleMoves_Compiled`** |
| **Nature Power / terrain→type** (`Cmd_callterrainattack`, **`settypetoterrain`**) | Optional ROM **30-byte** pack (**`FIRERED_ROM_BATTLE_TERRAIN_TABLE_PACK_OFF`**) | Compiled **`sNaturePowerMoves_Compiled` / `sTerrainToType_Compiled`** |
| **Heal / fly location coords** (`GetHealLocation`) | Optional ROM **`HEAL_LOCATION_COUNT × sizeof(struct HealLocation)`** | Compiled **`heal_locations.h`** |
| **Region map mapsec labels** (`GetMapName`) | Optional **pointer table** (**`FIRERED_ROM_REGION_MAP_MAPSEC_NAME_PTR_TABLE_OFF`**) or sparse profile string offsets; else **text-family** scan vs compiled **`gRegionMapMapsecNames_Compiled`** | Compiled pointers (same as pre-migration) |
| **Maps / layout / most `generated/data/*_portable*.c`** | **No** | Decomp snapshot |
| **`PORTABLE_FAKE_INCBIN` graphics** | **No** (placeholders + `_Portable`) | Huge surface |

## 4. Generic vs still compiled‑only (resolver token families)

| Token base | ROM overlay in resolver? |
|------------|---------------------------|
| `0x80` battle | **Yes** (ROM `u32` only if **`0x08……`–`0x09……`**; else compiled) |
| `0x81` event | **Yes** (from index **213**) |
| `0x82` field effect | **Yes** (ROM `u32` only if **`0x08……`–`0x09……`**; else compiled; not `FLDEFF_*` table) |
| `0x83` battle AI | **Yes** (full table) |
| `0x84` battle anim | **Yes** (ROM `u32` only if **`0x08……`–`0x09……`**; else compiled; not `gBattleAnims_General` §6e) |
| `0x85` map object | **Yes** (from index **0**) |

## 5. Targeted validation (no giant QA plan)

Use **narrow, existing** traces:

| Goal | How |
|------|-----|
| Patched bytes reach mapped ROM | **`FIRERED_TRACE_MOD_LAUNCH=1`** — handoff CRC/size; **`FIRERED_TRACE_ROM_CONSUMPTION=1`** — header bytes in mapped ROM (`docs/mods_runtime.md`) |
| Vanilla not regressed by overlays | **No env** for `FIRERED_ROM_*` overlays → code paths return NULL → **compiled** pointers only |
| Lighter hack (e.g. relink / FRLG+ style) | Same traces + optional **`FIRERED_ROM_EVENT_SCRIPT_PTRS_OFF`** etc. if scripts moved; Oak/text **scan** often still hits |
| Heavier hack (expanded ROM, engine fork) | `firered_portable_rom_compat` flags **`LIKELY_DECOMP_INCOMPATIBLE`** / **`REQUIRES_RUNTIME_SHIMS`** — expect **breakage outside** ROM slices; not fixable by one overlay |

*This pass is **architecture + trace design**; run the above locally on your ROM set to collect evidence.*

## 6. Biggest remaining compatibility blocker (evidence‑based)

**Primary:** The **bulk of gameplay data** still comes from **`cores/firered/generated/data/*_portable*.c`** + **`PORTABLE_FAKE_INCBIN`** + **`src_transformed/*_portable_assets.h`**, not from the cartridge. ROM overlays now cover **all six portable resolver token bases** (**`0x80000000` … `0x85000000`**), but **map headers, warps, wild data, move data, etc.** remain **decomp‑tied**. Multi‑hack compatibility **breaks** when hacks change **structure or counts** — **overlays cannot fix** that class of drift.

**Secondary:** **generated‑data tooling** or **narrow verticals** (graphics, cries, …) outside **`firered_portable_resolve_script_ptr`**.

## 7. Recommended next slice

1. **More structural ROM blobs** (same pattern as **`docs/portable_rom_species_national_dex_table.md`**) — e.g. Hoenn order tables, evolution tables, **when** layout matches the build.  
2. **Generated‑data / map pipeline** (regenerate or runtime-build **`generated/data`** from ROM) — **large** project.  
3. **Per-feature ROM graphics** (reduce **`PORTABLE_FAKE_INCBIN`** scope where binders exist).

## 8. Related docs

- `docs/rom_backed_runtime.md` — per‑vertical details  
- `docs/mods_runtime.md` — BPS/UPS + trace env vars  
- Per‑family: `docs/portable_rom_*.md`, `docs/portable_rom_event_script_pointer_overlay.md`, `docs/portable_rom_map_object_event_script_pointer_overlay.md`, `docs/portable_rom_battle_script_pointer_overlay.md`, `docs/portable_rom_field_effect_pointer_overlay.md`, `docs/portable_rom_battle_anim_pointer_overlay.md`, `docs/portable_rom_species_national_dex_table.md`, `docs/portable_rom_species_hoenn_dex_tables.md`, `docs/portable_rom_species_cry_id_table.md`, `docs/portable_rom_hm_moves_table.md`, `docs/portable_rom_deoxys_base_stats_table.md`, `docs/portable_rom_experience_tables.md`, `docs/portable_rom_type_effectiveness_table.md`, `docs/portable_rom_species_info_table.md`, `docs/portable_rom_tmhm_learnsets_table.md`, `docs/portable_rom_evolution_table.md`, `docs/portable_rom_battle_moves_table.md`, `docs/portable_rom_tutor_tables.md`, `docs/portable_rom_heal_locations_table.md`, `docs/portable_rom_region_map_section_grids.md`, `docs/portable_rom_region_map_section_layout.md`, `docs/portable_rom_region_map_fly_destinations.md`, `docs/next_rom_structural_seam_notes.md`, `docs/generated_data_rom_seam_playbook.md`
