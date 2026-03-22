# ROM-backed runtime migration (FireRed portable / SDL)

**Consolidation / matrix:** see **`docs/rom_compat_architecture.md`** (load path, priority rules, resolver token status, validation traces, recommended next slice).

## 1. Audit — largest compiled-data bypasses (by impact)

| Mechanism | Impact | Notes |
|-----------|--------|--------|
| **`PORTABLE_FAKE_INCBIN`** (`include/global.h`) | **Very high** | Strips all `INCBIN` binary embedding; forces alternate data paths. |
| **`src_transformed/*_portable_assets.h` + `#define gX gX_Portable`** | **Very high** | Title screen, party menu, help system, etc. use **vanilla** byte arrays, not cartridge. |
| **`cores/firered/generated/data/*_portable*.c`** | **Very high** | Maps, scripts, cries, battle data — **decomp snapshot**, not live ROM layout. |
| **`portable_script_pointer_resolver.c`** | **Very high** | Token → `gFireredPortable*Ptrs[]`; only raw ROM addresses hit `ENGINE_ROM_ADDR`. |
| **`rom_header_portable.c` (pre-migration)** | **Low–medium** | Hardcoded `BPRE` / `GAME_VERSION`; wrong for hacks + Mystery Gift metadata. |
| **M4A / sound tables** | **Medium** | Often portable or ROM-pointer mix; hack-dependent. |
| **`string_util` / `strings.c` paths** | **Medium–high** | Mix of `.rodata` and ROM string reads; per-call audit. |
| **Species ↔ National Dex** (`pokemon.c` tables) | **Medium–high** (first **structural** ROM slice) | Optional ROM **`u16[]`** when **`FIRERED_ROM_SPECIES_TO_NATIONAL_DEX_OFF`** / profile set; see §5c. |
| **Hoenn ↔ National dex tables** (`pokemon.c`) | **Medium–high** | Optional ROM **`u16[]`** pair; see §5d. |
| **Species → cry ID** (`SpeciesToCryId` / `cry_ids.h`) | **Medium** | Optional contiguous **`u16[]`**; see §5e. |
| **HM move ID list** (`IsHMMove2` / `sHMMoves`) | **Medium** | Optional **9 × `u16`** + **`0xFFFF`** sentinel; see §5f. |
| **Deoxys base stats** (`GetDeoxysStat` / `sDeoxysBaseStats`) | **Low–medium** | Optional **6 × `u16`** (FIRERED Attack forme); see §5g. |
| **Experience tables** (`gExperienceTables` / `ExperienceTableGet`) | **Medium–high** (first **medium flat `u32` grid**) | Optional **6 × (`MAX_LEVEL`+1) LE `u32`**, row-major + monotonic rows; see §5h. |
| **Type chart** (`gTypeEffectiveness` / `TYPE_EFFECT_*`) | **Medium–high** | Optional **336-byte** triplet table + structural validation; see §5i. |
| **Species info** (`gSpeciesInfo` / `struct SpeciesInfo`) | **High** (first **large structured** species table) | Optional **`NUM_SPECIES × 26`** byte wire blob + self-test + field validation; see §5j. |
| **Mon pic coords + enemy elevation** (`gMonFrontPicCoords` / `gMonBackPicCoords` / `gEnemyMonElevation`) | **Medium–high** (**`(SPECIES_UNOWN_QMARK+1) × 4 + NUM_SPECIES`** bytes, **no pointers**, broad battle/UI use) | Optional **`FIRERED_ROM_MON_PIC_LAYOUT_PACK_OFF`**; see **`docs/portable_rom_mon_pic_layout.md`**. |
| **TM/HM learnsets** (`sTMHMLearnsets` / `CanMonLearnTMHM`) | **Medium–high** (species-adjacent, **fixed `NUM_SPECIES × 8`** bytes) | Optional contiguous **`u32[2]`** rows + `SPECIES_NONE` check; see §5k. |
| **Egg moves** (`gEggMoves` / `GetEggMoves` in daycare) | **Medium** (variable-length **`u16`** stream, **no pointers**) | Optional **`FIRERED_ROM_EGG_MOVES_PACK_OFF`** pack + validator mirroring **`rom_blob_inspect.py --validate-egg-moves-u16`**; see **`docs/portable_rom_egg_moves_table.md`**. |
| **Level-up learnsets** (`gLevelUpLearnsets` / `gLevelUpLearnsets_Compiled`) | **High** (**`NUM_SPECIES`** pointers + variable **`u16`** lists) | Optional **`FIRERED_ROM_LEVEL_UP_LEARNSETS_PACK_OFF`** two-part pack (pointer table + blob); see **`docs/portable_rom_level_up_learnsets_family.md`**. |
| **Evolution table** (`gEvolutionTable` / `GetEvolutionTargetSpecies`, daycare, evolution scene) | **High** (species-adjacent, **fixed `NUM_SPECIES × EVOS_PER_MON × 6`** bytes) | Optional contiguous **`struct Evolution` wire** + structural validation; see §5l. |
| **Battle moves** (`gBattleMoves` / battle engine, summary, party PP) | **High** (fixed **`MOVES_COUNT × 9`** bytes, **no pointers**) | Optional **`struct BattleMove` wire** + field bounds + **`MOVE_NONE`** row; see §5m. |
| **Move names** (`gMoveNames` / UI, battle text, TM strings) | **Medium** (**`MOVES_COUNT × (MOVE_NAME_LENGTH+1)`**, **no pointers**) | Optional **`FIRERED_ROM_MOVE_NAMES_PACK_OFF`**; see **`docs/portable_rom_move_names_table.md`**. |
| **Battle Tower templates** (`gBattleTowerLevel50Mons` / **`gBattleTowerLevel100Mons`**, party fill) | **Medium–high** (**2 × 300 × `sizeof(struct BattleTowerPokemonTemplate)`**, **no pointers**) | Optional **`FIRERED_ROM_BATTLE_TOWER_MON_TEMPLATES_PACK_OFF`**; see **`docs/portable_rom_battle_tower_mon_templates.md`**. |
| **Move tutor** (`sTutorMoves` / `sTutorLearnsets`, party menu) | **Medium–high** ( **`TUTOR_MOVE_COUNT × u16`** + **`NUM_SPECIES × u16`**, **no pointers**) | Optional **paired** ROM blobs + bitmask / move-ID checks; see §5n. |
| **Heal / fly locations** (`sHealLocations`, `heal_locations.json` pipeline) | **Medium** (generated **map-adjacent** data, **`HEAL_LOCATION_COUNT × sizeof(struct HealLocation)`**) | Optional ROM blob; fly / heal lookup only; see §5o. |

## 2. Recommended migration order

1. **Cartridge header mirrors** (game code, software version, then title) — **small**, **reusable** (`ENGINE_ROM_ADDR` + offsets), **unblocks** any feature that branches on game code / version.
2. **One high-visibility graphics vertical** — e.g. title screen: stop `#define` to `*_Portable`, load from ROM via same addresses as retail (or LZ77 from ROM pointers). **High effort**, high payoff.
3. **Narrow script resolution** — e.g. resolve **one** script family from ROM offsets when pointer lies in `ENGINE_ROM` range, fallback to tokens. **Medium effort**, must stay compatible with saves.
4. **Regenerate or runtime-build `generated/data`** from loaded ROM — **large** tooling project.
5. **Reduce `PORTABLE_FAKE_INCBIN` scope** — only where a ROM-backed loader exists.

## 3. Implemented vertical slice (phase 1)

**ROM-backed cartridge header sync**

- After `engine_memory_init` copies the (possibly patched) ROM into `ENGINE_ROM_ADDR`, **`firered_portable_sync_rom_header_from_cartridge()`** copies:
  - **Game code** from ROM offset **0xAC** → `RomHeaderGameCode[4]`
  - **Software version** from ROM offset **0xBC** → `RomHeaderSoftwareVersion`
- Bounded by **`engine_memory_get_loaded_rom_size()`** so short buffers are not read past.
- **`RomHeaderGameCode` / `RomHeaderSoftwareVersion`** are no longer `const` in C headers so portable can refresh them after load.
- **Vanilla FireRed** unchanged in behavior (sync copies the same bytes that matched the old constants).

**Visible effect today:** mostly **Mystery Gift** (and any other code using these globals).

## 4. Title screen bypass (audit, pre–ROM graphics slice)

On **PORTABLE / SDL**, the title screen was wired as follows:

| Asset / path | Source |
|--------------|--------|
| **Game title logo** (8bpp BG0: pal + LZ tiles + LZ tilemap) | `#define` → `gGraphics_TitleScreen_GameTitleLogo{Pals,Tiles,Map}_Portable` in `src_transformed/title_screen_portable_assets.h` |
| **Box art mon, copyright strip, border BG, flames, slash, …** | Still `*_Portable` or local `*_Portable` / fake `INCBIN` in `title_screen.c` |
| **`graphics.c` INCBIN symbols** | `PORTABLE_FAKE_INCBIN` → empty placeholders; not used for logo once `title_screen.c` aliases to `_Portable` |
| **Generated `cores/firered/generated/data`** | Not used for this logo trio |

## 5. Implemented vertical slice (phase 2) — game title logo from ROM

**Slice:** **BG0 “Pokémon / FireRed” game title logo** only (palette + compressed tiles + compressed tilemap).

- **`firered_portable_title_screen_try_bind_game_title_logo()`** (`cores/firered/portable/firered_portable_title_screen_rom.c`):
  1. If **`FIRERED_ROM_TITLE_LOGO_PAL_OFF`**, **`FIRERED_ROM_TITLE_LOGO_TILES_OFF`**, **`FIRERED_ROM_TITLE_LOGO_MAP_OFF`** are all set (hex, `strtoul` syntax), uses those **file offsets** into the mapped ROM.
  2. Else **scans** the loaded ROM for the **vanilla pokefirered** LZ/pal fingerprints and **layout** (512-byte palette immediately before tiles LZ; map LZ **after** the compressed tiles, within **256KiB**: first match on **32-byte stock** compressed prefix, else **LZ77 `0x10`** with declared decompressed size **`0x500`** that fully parses — covers FRLG+-style repacking / padding).
- **`src_transformed/title_screen.c`** drops the three `#define`s for the logo and uses ROM pointers when bound, else **`_Portable` fallback** (vanilla still works if scan fails).
- **LZ77** is read through existing **`MallocAndDecompress` → `LZ77UnCompWram`** (ROM-backed `src` is fine).

**Visible when hacks matter:** Any loaded ROM that still matches the **layout + pal prefix** (vanilla order) but **replaced logo art** (different pixels after decompress) shows the **patched** logo. If a hack **rewrites** the compressed blobs such that the **first 32 bytes** of tiles (or pal prefix) change, the **scan may fail** → set the three **`FIRERED_ROM_TITLE_LOGO_*_OFF`** env vars for that ROM layout.

**Reuse pattern:** portable module + “bind once per title init” + env override + scan fallback; same idea can target other `*_Portable` families.

## 5c. Species → National Pokédex table (structural `u16[]`)

**Slice:** `SpeciesToNationalPokedexNum` / `NationalPokedexNumToSpecies` can use a **ROM-backed** copy of the decomp’s `sSpeciesToNationalPokedexNum[]` when configured.

- **`firered_portable_rom_species_national_dex_refresh_after_rom_load()`** runs after ROM map (with other portable refresh hooks). It copies **`(NUM_SPECIES - 1)`** little-endian **`u16`** values from **`FIRERED_ROM_SPECIES_TO_NATIONAL_DEX_OFF`** (env hex) or a **profile row** into a static cache.
- If the offset is unset, out of range, or the image is too small, the cache stays **inactive** and both functions use **compiled** rodata — **vanilla default unchanged**.
- **Limitation:** build-time **`NUM_SPECIES`** is still fixed; this slice does **not** support hacks that change the **number** of species without rebuilding.

See **`docs/portable_rom_species_national_dex_table.md`**.

## 5d. Hoenn dex tables (paired `u16[]`)

**Slice:** **`sSpeciesToHoennPokedexNum`** and **`sHoennToNationalOrder`** — optional ROM blobs **`FIRERED_ROM_SPECIES_TO_HOENN_DEX_OFF`** and **`FIRERED_ROM_HOENN_TO_NATIONAL_ORDER_OFF`** (each independent). See **`docs/portable_rom_species_hoenn_dex_tables.md`**.

## 5e. Species → cry ID (`u16[]` after OLD_UNOWN range)

**Slice:** **`FIRERED_ROM_SPECIES_CRY_ID_TABLE_OFF`** — **`FIRERED_ROM_SPECIES_CRY_ID_TABLE_ENTRY_COUNT`** little-endian **`u16`** values, index-aligned with **`sHoennSpeciesIdToCryId`** / **`cry_ids.h`**. See **`docs/portable_rom_species_cry_id_table.md`**.

## 5f. HM move list (`sHMMoves`, fixed 9 × `u16`)

**Slice:** **`FIRERED_ROM_HM_MOVES_TABLE_OFF`** — **9** little-endian **`u16`**, with **`0xFFFF`** present in-window (sentinel). Drives **`IsHMMove2`**. See **`docs/portable_rom_hm_moves_table.md`**.

## 5g. Deoxys base stats (`sDeoxysBaseStats`, 6 × `u16`)

**Slice:** **`FIRERED_ROM_DEOXYS_BASE_STATS_TABLE_OFF`** — **`NUM_STATS`** little-endian **`u16`** values for **`GetDeoxysStat`** (FIRERED Attack forme layout). See **`docs/portable_rom_deoxys_base_stats_table.md`**.

## 5h. Experience tables (`gExperienceTables`, 6 × (`MAX_LEVEL`+1) × `u32`)

**Slice:** **`FIRERED_ROM_EXPERIENCE_TABLES_OFF`** — **2424** bytes of little-endian **`u32`**, **6** growth-rate rows × **(`MAX_LEVEL` + 1)** level columns, **row-major**. Each row must be **monotonic non-decreasing** by level or the ROM path is rejected. All gameplay reads go through **`ExperienceTableGet`** (function on PORTABLE, macro elsewhere). See **`docs/portable_rom_experience_tables.md`**.

## 5i. Type effectiveness chart (`gTypeEffectiveness`, 336 bytes)

**Slice:** **`FIRERED_ROM_TYPE_EFFECTIVENESS_OFF`** — **336** raw **`u8`** bytes matching vanilla **triplet** layout (atk type, def type, multiplier), **`TYPE_FORESIGHT`** row, and **`TYPE_ENDTABLE`** terminator. Validated before activation; **`gTypeEffectivenessActivePtr`** selects ROM cache vs compiled on **PORTABLE**; **`TYPE_EFFECT_*`** macros read through it. See **`docs/portable_rom_type_effectiveness_table.md`**.

## 5j. Species info table (`gSpeciesInfo`, `NUM_SPECIES × 26` bytes)

**Slice:** **`FIRERED_ROM_SPECIES_INFO_TABLE_OFF`** — contiguous **`struct SpeciesInfo`** wire rows (**26** bytes each, **`NUM_SPECIES`** rows), decoded with explicit field layout + **Bulbasaur self-test** against compiled data + light per-row validation (types, growth rate). On **PORTABLE**, compiled rodata is **`gSpeciesInfo_Compiled[]`**; the **`gSpeciesInfo`** macro selects **`gSpeciesInfoActive`** (ROM cache) or compiled fallback. See **`docs/portable_rom_species_info_table.md`**.

## 5k. TM/HM learnset masks (`sTMHMLearnsets`, `NUM_SPECIES × 8` bytes)

**Slice:** **`FIRERED_ROM_TMHM_LEARNSETS_TABLE_OFF`** — **`NUM_SPECIES`** rows of **2 × little-endian `u32`** (64-bit TM/HM mask per species). **`SPECIES_NONE`** must be **(0, 0)**. **`gTMHMLearnsetsActive`** + **`sTMHMLearnsets`** macro in **`pokemon.c`**; compiled symbol **`sTMHMLearnsets_Compiled`**. See **`docs/portable_rom_tmhm_learnsets_table.md`**.

## 5l. Evolution table (`gEvolutionTable`, `NUM_SPECIES × EVOS_PER_MON × 6` bytes)

**Slice:** **`FIRERED_ROM_EVOLUTION_TABLE_OFF`** — **`NUM_SPECIES × 5`** evolution slots (**`struct Evolution`**: **3 × LE `u16`** per slot, **row-major**). Structural checks (`SPECIES_NONE` empty, **`method == 0`** padding, **`method` in 1…15**, valid **`targetSpecies`**). **`gEvolutionTableActive`** + **`gEvolutionTable`** macro in **`pokemon.h`**; compiled **`gEvolutionTable_Compiled`**. See **`docs/portable_rom_evolution_table.md`**.

## 5m. Battle moves (`gBattleMoves`, `MOVES_COUNT × 9` bytes)

**Slice:** **`FIRERED_ROM_BATTLE_MOVES_TABLE_OFF`** — **`MOVES_COUNT`** rows of **`struct BattleMove`** wire (**9** bytes each: effect, power, type, accuracy, pp, secondary chance, target, priority, flags). Per-row bounds (**types**, accuracy, PP, effect **`≤ EFFECT_CAMOUFLAGE`**, etc.) plus **`MOVE_NONE`** vanilla-shaped row. **`gBattleMovesActive`** + **`gBattleMoves`** macro in **`pokemon.h`**; compiled **`gBattleMoves_Compiled`**; **`rom_header_gf.c`** **`.moves`** uses **`gBattleMoves_Compiled`** on **PORTABLE**. See **`docs/portable_rom_battle_moves_table.md`**.

## 5m-ter. Battle terrain pack (Nature Power + terrain→type)

**Slice:** **`FIRERED_ROM_BATTLE_TERRAIN_TABLE_PACK_OFF`** — **30** bytes: **10 × LE `u16`** (Nature Power move per terrain **0..9**) then **10 × `u8`** (type per terrain for **`settypetoterrain`**). **All-or-nothing** validation; **`battle_script_commands.c`** selects ROM via **`NaturePowerMovesSelectTable()`** / **`TerrainToTypeSelectTable()`**. See **`docs/portable_rom_battle_terrain_tables.md`**.

## 5n. Move tutor tables (`sTutorMoves`, `sTutorLearnsets`)

**Slice:** **`FIRERED_ROM_TUTOR_MOVES_TABLE_OFF`** (**`TUTOR_MOVE_COUNT`** LE **`u16`**) and **`FIRERED_ROM_TUTOR_LEARNSETS_TABLE_OFF`** (**`NUM_SPECIES`** LE **`u16`**). **Both** must validate or **both** fall back to compiled. **`gTutorMovesActive` / `gTutorLearnsetsActive`** + macros in **`party_menu.c`** after **`tutor_learnsets.h`**. See **`docs/portable_rom_tutor_tables.md`**.

## 5o. Heal / fly locations (`sHealLocations`) + whiteout companions

**Slice:** **`FIRERED_ROM_HEAL_LOCATIONS_TABLE_OFF`** — **`HEAL_LOCATION_COUNT`** rows of **`struct HealLocation`** (wire = portable **`sizeof`**). **`gHealLocationsActive`** + macro in **`heal_location.c`**.

**Paired slice:** **`FIRERED_ROM_WHITEOUT_HEAL_CENTER_MAP_IDXS_OFF`** (**`HEAL_LOCATION_COUNT × 4`** LE **`u16`** pair per row) and **`FIRERED_ROM_WHITEOUT_HEALER_NPC_IDS_OFF`** (**`HEAL_LOCATION_COUNT`** **`u8`**). **Both** must validate or **both** use compiled **`sWhiteoutRespawnHealCenterMapIdxs_Compiled` / `sWhiteoutRespawnHealerNpcIds_Compiled`** (**`gWhiteoutRespawnHealCenterMapIdxsActive`** / **`gWhiteoutRespawnHealerNpcIdsActive`** + macros). Heal and whiteout ROM paths are **independent** (either can fall back without forcing the other). See **`docs/portable_rom_heal_locations_table.md`**.

## 5p. Region map section id grids (`sRegionMapSections_*`)

**Slice:** **`FIRERED_ROM_REGION_MAP_SECTION_GRIDS_PACK_OFF`** — **four** back-to-back tensors (**`REGION_MAP_SECTION_GRID_LAYERS × REGION_MAP_SECTION_GRID_HEIGHT × REGION_MAP_SECTION_GRID_WIDTH`** **`u8`** each, **2640** bytes total for vanilla dims). **All four** map views activate together or **all** fall back to compiled **`sRegionMapSections_*_Compiled`** (**`gRegionMapSectionGrid*Active`** + macros in **`region_map.c`**). See **`docs/portable_rom_region_map_section_grids.md`**.

## 5q. Region map section layout (`sMapSectionTopLeftCorners`, `sMapSectionDimensions`)

**Slice:** **`FIRERED_ROM_REGION_MAP_SECTION_LAYOUT_PACK_OFF`** — **`MAPSEC_COUNT × 2 × sizeof(u16)`** LE corner pairs, then the same size for dimension pairs (**`MAPSEC_COUNT × 8`** bytes total). **Both** tables activate together or **both** use compiled **`sMapSectionTopLeftCorners_Compiled` / `sMapSectionDimensions_Compiled`** (**`gMapSectionTopLeftCornersActive`** / **`gMapSectionDimensionsActive`** + macros in **`region_map.c`**). See **`docs/portable_rom_region_map_section_layout.md`**.

## 5r. Region map fly destinations (`sMapFlyDestinations`)

**Slice:** **`FIRERED_ROM_REGION_MAP_FLY_DESTINATIONS_TABLE_OFF`** — **`REGION_MAP_FLY_DESTINATION_ROW_COUNT × 3`** bytes (**`u8`** map group, **`u8`** map num, **`u8`** heal id per row). **`gMapFlyDestinationsActive`** + macro in **`region_map.c`**. See **`docs/portable_rom_region_map_fly_destinations.md`**.

## 5s. Region map mapsec display names (`GetMapName` / `sMapNames`)

**Slice:** **`REGION_MAP_MAPSEC_NAME_ENTRY_COUNT`** GBA-encoded, EOS-terminated strings (**`MAPSEC_NONE - KANTO_MAPSEC_START`** rows), bound into **`gRegionMapMapsecNamesResolved[]`**; **`region_map.c`** aliases **`sMapNames`** on **PORTABLE**. **Preferred:** **`FIRERED_ROM_REGION_MAP_MAPSEC_NAME_PTR_TABLE_OFF`** (or **`FireredRomU32TableProfileRow`**) → vanilla **`u32[]`** pointer table in ROM (**all-or-nothing** validated). **Else:** small **per-entry string file-offset** profile rows (in **`firered_portable_rom_region_map_mapsec_names.c`**), then **`firered_portable_rom_text_family_bind_all`** default scan vs compiled bytes (`scan_min_match_len = 6`). See **`docs/portable_rom_region_map_mapsec_names.md`**.

## 6b. Std scripts pointer table (`callstd` / `gotostd`)

The nine-entry **`gStdScripts`** ROM table (item obtain/find, msgbox std modes, etc.) can be read from the loaded ROM when **`FIRERED_ROM_STD_SCRIPTS_TABLE_OFF`** or a built-in profile row supplies the file offset. See `docs/portable_rom_std_scripts_table.md`.

## 6c. Special vars pointer table (`GetVarPointer` 0x8000–0x8014)

The 21-entry **`gSpecialVars`** ROM table (pointers to `gSpecialVar_0x8000` … `gSpecialVar_0x8014` in EWRAM) can be read when **`FIRERED_ROM_SPECIAL_VARS_TABLE_OFF`** or a profile row is set. Pointers must validate as **aligned EWRAM** addresses. See `docs/portable_rom_special_vars_table.md`.

**Shared plumbing:** `firered_portable_rom_u32_table.{h,c}` centralizes env + profile **table base** resolution, **little-endian `u32`** reads, and **ROM bounds** checks for these two tables. **Pointer semantics** (ROM script vs EWRAM) stay in each family module.

## 6d. Field effect script pointer table (`FieldEffectStart`)

The 70-entry table parallel to **`gFieldEffectScriptPointers`** (ROM addresses of field-effect bytecode, `FLDEFF_*` order) can be read from the loaded ROM when **`FIRERED_ROM_FIELD_EFFECT_SCRIPT_PTRS_OFF`** or a profile row is set. **`FieldEffectStart`** uses it when valid, else compiled pointers. See `docs/portable_rom_field_effect_script_pointers.md`.

Resolver tokens **`0x82000000 + i`** use a **different** parallel table (`gFireredPortableFieldEffectScriptPtrs`, portable token order) — §6k.

## 6g. Battle AI fragment pointer table (`0x83000000` resolver)

Portable tokens **`0x83000000 + i`** can resolve from a ROM **`u32[]` parallel to the full **`gFireredPortableBattleAiPtrs[]`** table (**`gFireredPortableBattleAiPtrCount`** entries) when **`FIRERED_ROM_BATTLE_AI_FRAGMENTS_TABLE_OFF`** or **`FIRERED_ROM_BATTLE_AI_FRAGMENT_PREFIX_OFF`** (legacy alias) is set. **`firered_portable_resolve_script_ptr`** tries ROM first, then compiled; **`firered_portable_fix_battle_ai_ptr`** is unchanged. If the ROM block is missing, OOB, or too small for the full table, entries fall back to compiled. See `docs/portable_rom_battle_ai_fragment_prefix.md`.

## 6h. Event script pointer overlay (`0x81000000` resolver)

Portable tokens **`0x81000000 + i`** can resolve from a ROM **`u32[]` parallel to **`gFireredPortableEventScriptPtrs[]`** (same length as **`gFireredPortableEventScriptPtrCount`**) when **`FIRERED_ROM_EVENT_SCRIPT_PTRS_OFF`** or a profile row is set. Indices **`0 .. 212`** mirror **`gScriptCmdTable`** (**host `ScrCmd_*` pointers**) and are **never** read from ROM; from index **`213`** onward, **`firered_portable_resolve_script_ptr`** tries the ROM overlay first, then compiled. See `docs/portable_rom_event_script_pointer_overlay.md`.

## 6i. Map object event script pointer overlay (`0x85000000` resolver)

Portable tokens **`0x85000000 + i`** can resolve from a ROM **`u32[]` parallel to **`gFireredPortableMapObjectEventScriptPtrs[]`** when **`FIRERED_ROM_MAP_OBJECT_EVENT_SCRIPT_PTRS_OFF`** or a profile row is set. The portable table is **sorted unique** object-event **`EventScript_*`** symbols only (**no** ScrCmd prefix); **`firered_portable_resolve_script_ptr`** tries the ROM overlay first for eligible indices, then compiled. See `docs/portable_rom_map_object_event_script_pointer_overlay.md`.

## 6j. Battle script pointer overlay (`0x80000000` resolver)

Portable tokens **`0x80000000 + i`** can resolve from a ROM **`u32[]` parallel to **`gFireredPortableBattleScriptPtrs[]`** when **`FIRERED_ROM_BATTLE_SCRIPT_PTRS_OFF`** or a profile row is set. The battle-script token table is **interleaved** (ROM bytecode, RAM `&g*`, `s*`/`c*`, …); the overlay **accepts only** **`0x08……`–`0x09……`** pointers within the loaded ROM — otherwise it falls back to compiled (so RAM/scripting slots stay correct). See `docs/portable_rom_battle_script_pointer_overlay.md`.

## 6k. Field-effect pointer overlay (`0x82000000` resolver)

Portable tokens **`0x82000000 + i`** can resolve from a ROM **`u32[]` parallel to **`gFireredPortableFieldEffectScriptPtrs[]`** when **`FIRERED_ROM_FIELD_EFFECT_PTRS_OFF`** or a profile row is set. This is **not** **`FIRERED_ROM_FIELD_EFFECT_SCRIPT_PTRS_OFF`** (§6d, **`FLDEFF_*`** order). The portable table mixes **host `FldEff_*`** and **ROM data** (palettes); the overlay **accepts only** **`0x08……`–`0x09……`** in-image pointers — use ROM word **`0`** for slots that must stay **compiled** natives. See `docs/portable_rom_field_effect_pointer_overlay.md`.

## 6l. Battle animation pointer overlay (`0x84000000` resolver)

Portable tokens **`0x84000000 + i`** can resolve from a ROM **`u32[]` parallel to **`gFireredPortableBattleAnimPtrs[]`** when **`FIRERED_ROM_BATTLE_ANIM_PTRS_OFF`** or a profile row is set. This is **not** **`FIRERED_ROM_BATTLE_ANIMS_GENERAL_PTRS_OFF`** (§6e, **`gBattleAnims_General`** / **`B_ANIM_*`**). The portable table mixes **host `AnimTask_*`**, script bytecode, and **ROM templates**; the overlay **accepts only** **`0x08……`–`0x09……`** in-image pointers — use ROM word **`0`** for host-only slots. See `docs/portable_rom_battle_anim_pointer_overlay.md`.

## 6f. Battle AI primary script table (`gBattleAI_ScriptsTable`)

The **30-entry** top-level AI script pointer table (`AI_CheckBadMove` … `AI_FirstBattle`) can be read from ROM when **`FIRERED_ROM_BATTLE_AI_SCRIPTS_TABLE_OFF`** or a profile row is set. **`BattleAI_DoAIProcessing`** (`AIState_SettingUp`) uses it when valid; embedded **`0x83…`** script tokens still go through **`firered_portable_resolve_script_ptr`**. See `docs/portable_rom_battle_ai_scripts_table.md`.

## 6e. General battle animation script pointer table (`LaunchBattleAnimation`)

The 28-entry **`gBattleAnims_General`** table (weather ticks, Safari rock/reaction, item effects, etc.) can be read from ROM when **`FIRERED_ROM_BATTLE_ANIMS_GENERAL_PTRS_OFF`** or a profile row is set. **`LaunchBattleAnimation`** uses it when the table argument is **`gBattleAnims_General`**; otherwise behavior is unchanged. See `docs/portable_rom_battle_anims_general_table.md`. Resolver tokens **`0x84000000 + i`** use a **separate** parallel table — §6l.

## 6a. ROM asset profile table (built-in offsets)

After compat classification, optional **static** rows in `firered_portable_rom_asset_profile.c` can map
(`rom_crc32` and/or `profile_id`) → file offsets for specific `FireredRomAssetFamily` values. Family binders
use **env first**, then **profile**, then **scan**, then **`*_Portable`**. See `docs/portable_rom_asset_bind.md`.

## 6. Environment / tracing

- `FIRERED_TRACE_MOD_LAUNCH=1` — SDL shell: patch apply + **`engine_load_rom`** handoff (ptr/size/**CRC32**), success/failure (`engine/shells/sdl/main.c`). Use to confirm **patched ROM bytes** reach the core.
- `FIRERED_TRACE_ROM_CONSUMPTION=1` — raw GBA header bytes in mapped ROM (`engine_memory.c`).
- `FIRERED_TRACE_TITLE_ROM=1` (non-empty, not `0`) — stderr diagnostics for **every** title-logo bind attempt: env/scan success, env incomplete or OOB, scan statistics, explicit **`fallback: ... *_Portable`** when ROM bind fails (`firered_portable_title_screen_rom.c`). Not gated on `NDEBUG` (works in release builds too).

## 7. FRLG+ early-visible audit (offline)

For **FireRed & LeafGreen+** (`mods/frlgPlus.bps` on clean `baserom.gba`), offline checks show the **shipping title graphics** (logo, copyright strip, box art, background) and **oak speech BG** tiles/tilemaps still embed the **same bytes** as vanilla `graphics/` — only file offsets in the ROM move. The **GBA internal name** stays `POKEMON FIRE` / game code `BPRE`. So “title still looks vanilla” is largely **because the hack keeps vanilla art there**, not because ROM bind failed.

**Still compiled / mirrored on PORTABLE** after the title-logo slice: all other title layers (`*_Portable` in `src_transformed/title_screen.c`), **all** oak intro **graphics** (`oak_speech_portable_assets.h`), and **all** oak intro **strings** (`gOakSpeech_Text_*` from `data/text/new_game_intro.inc`). **Next high-visibility ROM-backed target** for hacks that change dialogue: **oak speech text** (bounded string family) or, for overworld changes, **scripts/maps** (`cores/firered/generated/data/*_portable*` + `portable_script_pointer_resolver.c`).

Helper: `python tools/frlgplus_early_visible_audit.py` — reapplies BPS and reports whether listed vanilla graphics files appear verbatim in stock vs patched ROM.

## 8. Oak intro text ROM slice (PORTABLE / SDL)

**Slice:** `gOakSpeech_Text_*` (Oak’s intro dialogue) resolves from the **mapped ROM** when possible.

- **Shared helper:** `firered_portable_rom_text_family_bind_all()` (`firered_portable_rom_text_family.c`) — see `docs/portable_rom_text_family.md`.
- **`firered_portable_oak_speech_get_text()`** (`cores/firered/portable/firered_portable_oak_speech_text_rom.c`) fills a static cache via that helper:
  1. Optional env **`FIRERED_ROM_OAK_TX_<Suffix>`** (hex file offset, `strtoul` syntax) — e.g. `FIRERED_ROM_OAK_TX_WelcomeToTheWorld=0x...`. Accepts **edited** strings if a terminating `EOS` (0xFF) exists within the ROM within ~2KiB.
  2. Optional **profile** callback (not wired for Oak yet; same hook as other text families).
  3. Else **full-string scan** of the ROM vs the **compiled decomp** bytes (same encoding as retail). Works when text is **unchanged** but **relocated** (typical relink hacks).
  4. Else **compiled fallback** (current behavior).

- **`src_transformed/oak_speech.c`** (PORTABLE only): `#define gOakSpeech_Text_*` → `firered_portable_oak_speech_get_text(...)`.

**Trace:** `FIRERED_TRACE_OAK_SPEECH_ROM=1` — one stderr line summarizing env / profile / scan / fallback counts (not gated on `NDEBUG`).

## 9. Main menu palette ROM slice (PORTABLE / SDL)

**Slice:** **Continue / main menu** BG palette slots loaded from the mapped ROM when bind succeeds (`graphics/main_menu/bg.gbapal` + `textbox.gbapal`, 16 colors each).

- **`firered_portable_main_menu_try_bind_palettes()`** (`cores/firered/portable/firered_portable_main_menu_pal_rom.c` via `firered_portable_rom_asset_bind_run`):
  1. Env **`FIRERED_ROM_MAIN_MENU_BG_PAL_OFF`** and **`FIRERED_ROM_MAIN_MENU_TEXTBOX_PAL_OFF`** (hex); **both** required.
  2. Else **32-byte signature scan** (vanilla palette prefixes) when the compat profile allows ROM scans, or when **`FIRERED_ROM_MAIN_MENU_PAL_SCAN`** is set.
- **`src_transformed/main_menu.c`**: bind **once** (same pattern as Oak main BG); **`LoadPalette`** uses ROM pointers when non-NULL, else **`main_menu_portable_assets.h`** mirrors.

**Trace:** `FIRERED_TRACE_MAIN_MENU_PAL_ROM=1`.

### Portable Pokémon graphics (`graphics/pokemon/...`) and working directory

SDL loads compressed front/back pics and normal/shiny palettes from **disk** via relative paths (`pokemon_graphics_portable_paths.h`). If the process **current working directory** is not the repo root (common when launching `decomp_engine_sdl.exe` from `build/` or with a mod launcher), `fopen` can fail → portable resolver returned **NULL** → `LoadCompressedSpritePaletteUsingHeap` crashed dereferencing NULL.

**Mitigations in tree:** (1) `ResolveCompressedSprite*` falls back to `src->data` when portable load returns NULL (fake INCBIN placeholder; avoids the crash). (2) Optional env **`FIRERED_ASSET_ROOT`** — absolute path to the **pokefirered repo root** — is tried first when opening `graphics/pokemon/...` files. Example (PowerShell):  
`$env:FIRERED_ASSET_ROOT="C:\path\to\pokefirered-master"`
