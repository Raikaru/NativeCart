# ROM-backed runtime migration (FireRed portable / SDL)

**Consolidation / matrix:** see **`docs/rom_compat_architecture.md`** (load path, priority rules, resolver token status, validation traces, recommended next slice).

**Native host prior art (optional, not ROM data):** SDL/main-loop and hardware-shim patterns from **other** Gen III decomp PC ports (e.g. Emerald **`pc-port`** forks) are summarized under **`docs/rom_compat_architecture.md`** §G and **`docs/cfru_upstream_reference.md`** §6. Use those for **platform structure**, not for FireRed ROM layout or hack table formats.

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
| **Facility class lookups** (`gFacilityClassToPicIndex` / `gFacilityClassToTrainerClass`, tower/link/overworld) | **Medium** (**`2 × NUM_FACILITY_CLASSES`** **`u8`**, **no pointers**, not `gTrainers`) | Optional **`FIRERED_ROM_FACILITY_CLASS_LOOKUPS_PACK_OFF`**; see **`docs/portable_rom_facility_class_lookups.md`**. |
| **TM/HM learnsets** (`sTMHMLearnsets` / `CanMonLearnTMHM`) | **Medium–high** (species-adjacent, **fixed `NUM_SPECIES × 8`** bytes) | Optional contiguous **`u32[2]`** rows + `SPECIES_NONE` check; see §5k. |
| **Egg moves** (`gEggMoves` / `GetEggMoves` in daycare) | **Medium** (variable-length **`u16`** stream, **no pointers**) | Optional **`FIRERED_ROM_EGG_MOVES_PACK_OFF`** pack + validator mirroring **`rom_blob_inspect.py --validate-egg-moves-u16`**; see **`docs/portable_rom_egg_moves_table.md`**. |
| **Level-up learnsets** (`gLevelUpLearnsets` / `gLevelUpLearnsets_Compiled`) | **High** (**`NUM_SPECIES`** pointers + variable **`u16`** lists) | Optional **`FIRERED_ROM_LEVEL_UP_LEARNSETS_PACK_OFF`** two-part pack (pointer table + blob); see **`docs/portable_rom_level_up_learnsets_family.md`**. |
| **Evolution table** (`gEvolutionTable` / `GetEvolutionTargetSpecies`, daycare, evolution scene) | **High** (species-adjacent, **fixed `NUM_SPECIES × EVOS_PER_MON × 6`** bytes) | Optional contiguous **`struct Evolution` wire** + structural validation; see §5l. |
| **Battle moves** (`gBattleMoves` / battle engine, summary, party PP) | **High** (fixed **`MOVES_COUNT × 9`** bytes, **no pointers**) | Optional **`struct BattleMove` wire** + field bounds + **`MOVE_NONE`** row; see §5m. |
| **Move names** (`gMoveNames` / UI, battle text, TM strings) | **Medium** (**`MOVES_COUNT × (MOVE_NAME_LENGTH+1)`**, **no pointers**) | Optional **`FIRERED_ROM_MOVE_NAMES_PACK_OFF`**; see **`docs/portable_rom_move_names_table.md`**. |
| **Battle Tower templates** (`gBattleTowerLevel50Mons` / **`gBattleTowerLevel100Mons`**, party fill) | **Medium–high** (**2 × 300 × `sizeof(struct BattleTowerPokemonTemplate)`**, **no pointers**) | Optional **`FIRERED_ROM_BATTLE_TOWER_MON_TEMPLATES_PACK_OFF`**; see **`docs/portable_rom_battle_tower_mon_templates.md`**. |
| **Move tutor** (`sTutorMoves` / `sTutorLearnsets`, party menu) | **Medium–high** ( **`TUTOR_MOVE_COUNT × u16`** + **`NUM_SPECIES × u16`**, **no pointers**) | Optional **paired** ROM blobs + bitmask / move-ID checks; see §5n. |
| **Heal / fly locations** (`sHealLocations`, `heal_locations.json` pipeline) | **Medium** (generated **map-adjacent** data, **`HEAL_LOCATION_COUNT × sizeof(struct HealLocation)`**) | Optional ROM blob; fly / heal lookup only; see §5o. |
| **Map border connections** (`gMapHeader.connections`, `data/maps/**/map.json` **connections**) | **Medium** (dense **`MAP_GROUPS_COUNT × 256`** rows, **100** bytes each, max **8** **`struct MapConnection`** per map, **no pointers** in the pack) | Optional **`FIRERED_ROM_MAP_CONNECTIONS_PACK_OFF`**; merge on load; see **`docs/portable_rom_map_connections_pack.md`**. |
| **Trainer money multipliers** (`gTrainerMoneyTable`, post-battle payout) | **Low–medium** (**≤256** × **`struct TrainerMoney`** (**2** bytes), **0xFF** terminator) | Optional **`FIRERED_ROM_TRAINER_MONEY_TABLE_OFF`** → **`gTrainerMoneyTable_Compiled`**; see **`docs/portable_rom_trainer_money_table.md`**. |
| **Species display names** (`gSpeciesNames`, GBA-encoded grid) | **Medium** (**412×11** bytes for vanilla **`gSpeciesNames_Compiled`** shape) | Optional **`FIRERED_ROM_SPECIES_NAMES_PACK_OFF`**; see **`docs/portable_rom_species_names_pack.md`**. |
| **Map header script table** (`MapHeader.mapScripts`, tagged stream) | **Medium** (dense **`MAP_GROUPS_COUNT × 256 × 8`** directory + variable blobs; **no** mixed per-map without **`0/0`** row) | Optional **`FIRERED_ROM_MAP_SCRIPTS_DIRECTORY_PACK_OFF`**; see **`docs/portable_rom_map_scripts_directory.md`**. |
| **Map events aggregate** (`MapHeader.events` — objects / warps / coord / bg) | **High** (same grid as map scripts directory + per-map blob; mixed pointer encoding via **`firered_portable_resolve_script_ptr`**) | Optional **`FIRERED_ROM_MAP_EVENTS_DIRECTORY_PACK_OFF`**; see **`docs/portable_rom_map_events_directory.md`**. |
| **Wild encounter family** (`gWildMonHeaders` graph — land / water / rock smash / fishing) | **High** (WINF pack + GBA **`0x08……`** pointer closure; host rebuild into malloc’d **`struct WildPokemonHeader`**) | Optional **`FIRERED_ROM_WILD_ENCOUNTER_FAMILY_PACK_OFF`**; **`FireredWildMonHeadersTable()`** in **`wild_encounter.c`** / **`wild_pokemon_area.c`**; see **`docs/portable_rom_wild_encounter_family.md`**. |

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

## 5u. Trainer money table (`gTrainerMoneyTable`)

**Slice:** **`FIRERED_ROM_TRAINER_MONEY_TABLE_OFF`** — **`struct TrainerMoney`** stream (**`u8 classId`**, **`u8 value`**) ending with **`classId == 0xFF`**. **`firered_portable_rom_trainer_money_table_refresh_after_rom_load()`**; **`gTrainerMoneyTable`** macro in **`battle_main.h`**. See **`docs/portable_rom_trainer_money_table.md`**.

## 5v. Species names pack (`gSpeciesNames`)

**Slice:** **`FIRERED_ROM_SPECIES_NAMES_PACK_OFF`** — **`(SPECIES_CHIMECHO + 1) × (POKEMON_NAME_LENGTH + 1)`** bytes, row-major **`u8`**, EOS-in-row validation. **`gSpeciesNames`** macro in **`pokemon.h`**. See **`docs/portable_rom_species_names_pack.md`**.

## 5w. Map header scripts directory (`MapHeader.mapScripts`)

**Slice:** **`FIRERED_ROM_MAP_SCRIPTS_DIRECTORY_PACK_OFF`** — **`MAP_GROUPS_COUNT × 256 × 8`** bytes (**LE `u32` blob file offset + LE `u32` length**). Non-**`0/0`** rows point **`mapScripts`** into the mapped ROM; **`0/0`** keeps compiled **`map_data_portable`**. Structural validation of the tagged 5-byte table walk. See **`docs/portable_rom_map_scripts_directory.md`**.

## 5x. Map events directory (`MapHeader.events`)

**Slice:** **`FIRERED_ROM_MAP_EVENTS_DIRECTORY_PACK_OFF`** — same **8-byte** directory rows as map scripts; each non-**`0/0`** row points at a validated **`MPEV`** version-**1** blob describing the full **`MapEvents`** aggregate (object / warp / coord / bg wire rows). **`firered_portable_rom_map_events_directory_merge`** overwrites **`gMapHeader.events`** after the compiled header copy when active. See **`docs/portable_rom_map_events_directory.md`**.

## 5t. Map connections pack (`gMapHeader.connections`)

**Slice:** **`FIRERED_ROM_MAP_CONNECTIONS_PACK_OFF`** — dense grid **`MAP_GROUPS_COUNT × 256`** rows of **100** bytes (**`u32`** **`count`** in low **8** bits + **8 × 12**-byte **`struct MapConnection`** wire slots). **All-or-nothing** validation; **`firered_portable_rom_map_connections_merge`** overwrites **`gMapHeader.connections`** after **`gMapGroups`** copy when active. **Compiled fallback:** **`map_data_portable`** headers. See **`docs/portable_rom_map_connections_pack.md`**.

## 5y. Wild encounter family (`gWildMonHeaders` + sub-tables)

**Slice:** **`FIRERED_ROM_WILD_ENCOUNTER_FAMILY_PACK_OFF`** — **`WINF`** magic + version **`1`** + **`header_bytes`**, then **20-byte** GBA-wire **`WildPokemonHeader`** rows (**`u8`** group/num, **2** zero pad bytes, **4** LE **`u32`** pointers). **All-or-nothing:** **`firered_portable_rom_wild_encounter_family_refresh_after_rom_load()`** validates pointer closure and species bounds, allocates host **`WildPokemonHeader` / `WildPokemonInfo` / `WildPokemon`**, then **`FireredWildMonHeadersTable()`** routes **`wild_encounter.c`** and **`wild_pokemon_area.c`** away from compiled **`gWildMonHeaders`**. **Compiled fallback** when the offset is unset or validation fails. See **`docs/portable_rom_wild_encounter_family.md`**. Tooling: **`tools/portable_generators/build_wild_encounter_family_pack.py`** + **`validate_wild_encounter_family_pack.py`**.

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

**Still compiled / mirrored on PORTABLE** after the title-logo slice: all other title layers (`*_Portable` in `src_transformed/title_screen.c`). The **earliest new-game scene** (`StartNewGameScene` → `Task_NewGameScene`) shows the **controls guide** first, then **Pikachu intro** pages (unless §8c’s optional ROM prose pointer table is active — then **N generic prose pages** replace the Pikachu intro segment). On PORTABLE, those strings resolve through **`firered_portable_early_new_game_help_get_text()`** (§8b — env / scan / compiled fallback from **`data/text/new_game_intro.inc`**). The same path still loads **BG LZ tiles/tilemaps** and **Pikachu** graphics from **`oak_speech_portable_assets.h`** (`*_Portable`); Oak’s **later** main speech background may bind from the loaded ROM via **`firered_portable_oak_speech_try_bind_main_bg`** (env/scan; **`docs/portable_rom_oak_speech_bg.md`**).

**Oak’s spoken dialogue** (`gOakSpeech_Text_*`) uses **`firered_portable_oak_speech_get_text()`** (§8). **Next gap** for full early-intro parity with some hacks: **intro graphics** from ROM, or overworld **scripts/maps** when the intro is script-driven.

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

### 8b. Controls guide / Pikachu intro text (PORTABLE / SDL)

**Slice:** **`gControlsGuide_Text_*`** (intro + pages 2–3 button copy) and **`gPikachuIntro_Text_Page{1,2,3}`** — the text shown before Oak’s spoken lines.

- **`firered_portable_early_new_game_help_get_text()`** (`cores/firered/portable/firered_portable_early_new_game_help_text_rom.c`) — same **`firered_portable_rom_text_family`** flow as §8.
- **Env (hex file offsets):** `FIRERED_ROM_CTRL_GUIDE_TX_Intro`, `_DPad`, `_AButton`, `_BButton`, `_StartButton`, `_SelectButton`, `_LRButtons`; `FIRERED_ROM_PIKACHU_INTRO_TX_Page1` … **`Page3`**.
- **`src_transformed/oak_speech.c`:** PORTABLE call sites use the helper; non-PORTABLE keeps direct **`gControlsGuide_*` / `gPikachuIntro_*`** symbols.

**Trace:** `FIRERED_TRACE_EARLY_NEW_GAME_HELP_ROM=1`.

### 8c. Optional new-game intro prose pointer table (PORTABLE / SDL)

**Generic opt-in seam:** hacks can replace the vanilla **Pikachu intro** segment with **N** full-screen GBA-encoded text pages (EOS-terminated), without script/scene parsing. The runtime only reads a **contiguous `u32[]`** of ROM pointers plus an explicit page count; invalid or unset config leaves the decomp flow unchanged.

- **Module:** `firered_portable_new_game_intro_prose_rom_should_run()` / `_page_count()` / `_get_page()` (`cores/firered/portable/firered_portable_new_game_intro_prose_rom.c`).
- **Env (highest priority; both required when using env):**
  - **`FIRERED_ROM_NEW_GAME_INTRO_PROSE_PTR_TABLE_OFF`** — hex **file offset** of **`page_count`** little-endian **`u32`** values. Each word must be a ROM address in the usual **`0x08XXXXXX`** ROM region; the corresponding **file offset** `word - 0x08000000` must lie in the loaded image.
  - **`FIRERED_ROM_NEW_GAME_INTRO_PROSE_PAGE_COUNT`** — page count (`strtoul(..., 0)`), **1 … 16** (`FIRERED_NEW_GAME_INTRO_PROSE_MAX_PAGES`).
- **Profile (when env is unset):** small built-in rows in `firered_portable_new_game_intro_prose_rom.c`, matched on **`firered_portable_rom_compat_get()`** — same **IEEE CRC32** (full mapped image) and optional **`profile_id`** rules as `FireredRomU32TableProfileRow` in `firered_portable_rom_u32_table.c`. Today this covers the **offline CI fixture** ROM produced by `tools/build_offline_layout_prose_fixture.py` (see §8d).
- **Validation:** table and every string must fit in the mapped ROM; each string must reach **EOS (0xFF)** within **8192** bytes (`FIRERED_NEW_GAME_INTRO_PROSE_MAX_STR`). Any failure → seam **inactive** (same as unset env).
- **UI hook:** `src_transformed/oak_speech.c` — `Task_ControlsGuide_Clear` (PORTABLE): if active, it runs **`Task_NewGameIntroProse_*`** instead of **`Task_PikachuIntro_LoadPage1`**, reusing the same presentation and **`gMain.state`** transitions as the Pikachu intro (**BG tilemap**, Pikachu OBJs, textbox, top bar, cursor, WIN0, **`PIKACHU_INTRO_PRINT_PAGE_TEXT` / `PIKACHU_INTRO_FADE_IN_PAGE`** blend via **`BLDCNT`/`BLDALPHA`**). Data contract is unchanged.
- **Presentation trace (debug):** `FIRERED_TRACE_INTRO_PRESENTATION=1` emits the same intro-presentation lines to **`stderr`** (always, when the env is set) and also via **`firered_runtime_trace_external`** when the engine trace ring is active (`NATIVECART_TRACE`, non-**`NDEBUG`**). Use **`stderr` redirection** (e.g. `2> trace.txt` or `2>&1`) so one run captures **`introPresentation[...]`** alongside **`RomAutoPrep:`** lines without depending on the native-cart trace mask.

**Hack workflow:** ship or inject a **small free-space table** of pointers to your strings (or build it in a patch); for ad hoc testing set the two env vars when launching SDL, or add a **profile row** (CRC and/or `profile_id`) so known builds load without env. This deliberately does **not** parse mixed script/scene blobs — only an explicit pointer list.

### 8d. Profile auto-bind (layout companion + intro prose)

**Identity:** `firered_portable_rom_compat_refresh_after_rom_load()` already records **`rom_crc32`** (IEEE CRC of the **entire** loaded ROM image) and **`profile_id`** from a small **`KnownProfileRow`** table in `firered_portable_rom_compat.c` (game code, internal title needle, and/or explicit CRC). Layout pack binders reuse the same CRC / `profile_id` matching via `firered_portable_rom_u32_table_profile_lookup()` / `firered_portable_rom_u32_table_resolve_base_off()` (**env first**, then profile, then inactive).

**What auto-binds without env today**

| Seam | Module / env vars | Built-in profile rows |
|------|-------------------|------------------------|
| Layout companion (metatile directory + dimensions + tileset indices) | `FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF`, `…_DIMENSIONS_…`, `…_TILESET_INDICES_…` | `firered_portable_rom_map_layout_metatiles.c` — **two** CRC-only rows for the offline CI images (layout-only and layout+prose fixture); offsets match `build/offline_map_layout_rom_companion_bundle.bin.manifest.txt` with bundle base **`0`** (see `tools/run_offline_build_gates.py`). |
| Intro prose pointer table | `FIRERED_ROM_NEW_GAME_INTRO_PROSE_PTR_TABLE_OFF`, `…_PAGE_COUNT` | `firered_portable_new_game_intro_prose_rom.c` — **one** CRC-only row for the **layout+prose** fixture (`tools/build_offline_layout_prose_fixture.py`). |

**Overrides:** per-family **`FIRERED_ROM_*`** env vars remain **higher priority** than profile rows. Partial env (e.g. prose table offset without page count) leaves the seam inactive.

**Maintainers:** when `build_map_layout_rom_companion_bundle.py` output or the offline splice host changes, **CRC32** values drift — `tools/build_offline_layout_prose_fixture.py` fails until you update the goldens in that script and the matching **`0x…`** constants in the two C files above (and optional `KnownProfileRow` labels in `firered_portable_rom_compat.c`).

**Still manual for most real hacks:** retail or patched ROMs that are **not** the offline fixture need **env** (e.g. `layout_rom_companion_emit_embed_env.py`) and/or **new profile rows** (CRC and/or stable `profile_id` from `firered_portable_rom_compat.c`). Omega-style validation with **custom** string offsets continues to use **`tools/append_intro_prose_ptr_table_to_rom.py`** plus env or a **dedicated** profile row once you record that ROM’s CRC.

### 8e. Promoting a manually tested ROM to a profiled image (developers)

**Goal:** you already validated a ROM under SDL with **`FIRERED_ROM_*`** env vars; you want the same image to **auto-bind** without env by adding small rows to the portable core.

1. **Freeze the exact file** you tested (same bytes → same **IEEE CRC32**). Padding or re-saving the ROM changes CRC and breaks CRC-only rows.
2. **Capture identity + snippets:**  
   `python tools/capture_portable_rom_profile.py path/to/your.gba --profile-id short_slug --label "Readable name"`  
   Optionally pass the same offsets you used in the shell (from `layout_rom_companion_emit_embed_env.py`, `append_intro_prose_ptr_table_to_rom.py`, etc.):  
   `--layout-meta-off … --layout-dim-off … --layout-ts-off …` and/or `--prose-table-off … --prose-pages N`.  
   The script prints **CRC32**, **game code**, **internal title**, and **paste-ready C fragments** for `KnownProfileRow` and the seam tables.
3. **Merge by hand:** insert the **`KnownProfileRow`** near the top of `s_known_profiles[]` in `firered_portable_rom_compat.c` (CRC-specific rows **before** generic `crc32 == 0` rows). Append **`FireredRomU32TableProfileRow`** lines to the three layout arrays in `firered_portable_rom_map_layout_metatiles.c` (and any other layout seam arrays you actually use). Append **`FireredRomIntroProseProfileRow`** in `firered_portable_new_game_intro_prose_rom.c` if applicable.
4. **Rebuild SDL** and load the ROM **with no `FIRERED_ROM_*` vars** for those seams; confirm behavior matches the env-driven run.
5. **Optional:** add `--json` for machine-readable output (CI or notes).

**Offline sanity check:** after `make check-offline-gates` / `python tools/run_offline_build_gates.py`, run:

`python tools/capture_portable_rom_profile.py build/offline_splice_test_out.bin` → CRC **`0x33B8FD06`**.

`python tools/capture_portable_rom_profile.py build/offline_layout_prose_fixture.bin --layout-meta-off 0 --layout-dim-off 0x788DE --layout-ts-off 0x794D6 --prose-table-off 0x80006 --prose-pages 3` → CRC **`0x217A04B1`** and snippets consistent with the checked-in rows.

### 8f. Auto-prepare on load (raw Omega → profiled image in memory)

**Problem:** layout companion + intro prose profiles key off the **prepared** ROM’s IEEE CRC (`0x8D911746` layout-only, `0x3479F247` layout + prose). A **raw** 16 MiB `Pokemon Fire Red Omega.gba` has a different CRC and no spliced bundle.

**Approach:** `firered_portable_rom_auto_prepare()` runs in `engine_backend_init` **before** `engine_runtime_copy_rom`. When the incoming image matches a tiny built-in rule (**16 MiB**, CRC **`0xB197A9B9`** for the stock Omega dump used here), the engine:

1. Loads a **local-generated** **`omega_map_layout_rom_companion_bundle.bin`** (map layout companion built with **`--metatile-pack-base-offset 0x1000000`**, same wire as offline bundle but relocated). Keep this blob **out of git**; it is a local ROM-derived artifact.
2. Allocates **`max(rom_size, 0x1000000 + bundle_bytes)`**, copies the ROM, splices the bundle at **`0x01000000`**.
3. Unless disabled, appends the **3×`u32`** intro-prose pointer table at **EOF** using the same string file offsets as §8c (`0x71A78D`, `0x71AA66`, `0x71A972`).

The resulting buffer’s CRC matches the existing profile rows, so **layout + prose auto-bind without env** and without a second “prepared” ROM file on disk.

**Bundle discovery (first hit wins):** `FIRERED_OMEGA_LAYOUT_COMPANION_BUNDLE_PATH`, then `FIRERED_ASSET_ROOT` + `cores/firered/portable/data/…` or `build/…`, then a few **relative** paths (repo root vs `build/` cwd).

**Escape hatches:** `FIRERED_ROM_AUTO_PREPARE_DISABLE` — skip all auto-prep. `FIRERED_AUTO_PREPARE_SKIP_INTRO_PROSE` — splice layout only (CRC **`0x8D911746`**). Env vars for layout/prose still override profile rows after prep.

**Trace:** `FIRERED_TRACE_ROM_AUTO_PREPARE=1` — stderr one-liner (output size/CRC, bundle path, prose on/off).

**Regenerating the shipped bundle** (clobbers intermediate `build/offline_map_layout_*` packs; rebuild offline bundle afterward if you need base-0 CI artifacts):

`python tools/portable_generators/build_map_layout_rom_companion_bundle.py . build/omega_map_layout_rom_companion_bundle.bin --metatile-pack-base-offset 0x1000000`  
keep `build/omega_map_layout_rom_companion_bundle.bin` local, or copy it into **`cores/firered/portable/data/`** only as an **ignored local file** next to the committed `.manifest.txt`; then restore **`build/offline_map_layout_rom_companion_bundle.bin`** with **`--metatile-pack-base-offset 0x0`**.

**Local validation recipe (Fire Red Omega test data, generic seam):** use your own copy of **`Pokemon Fire Red Omega.gba`** or **`build/omega_layout_test.gba`** (not redistributed here). Append a **`3 × u32`** table at EOF with:

`python tools/append_intro_prose_ptr_table_to_rom.py "Pokemon Fire Red Omega.gba" -o build/omega_intro_prose_test.gba --string-off 0x71A78D --string-off 0x71AA66 --string-off 0x71A972`

The script prints **`FIRERED_ROM_NEW_GAME_INTRO_PROSE_PTR_TABLE_OFF`** (the pre-append ROM size) and **`FIRERED_ROM_NEW_GAME_INTRO_PROSE_PAGE_COUNT=3`**. Launch **`build/decomp_engine_sdl.exe`** on that output ROM with those two vars plus whatever **`FIRERED_ROM_*`** layout companion bundle you already use for that hack under SDL; start a new game and confirm the three prose pages appear in place of the Pikachu intro segment (after controls), then A through to Oak. Omit the two prose env vars to confirm vanilla fallback.

**SDL build:** the shell needs SDL3 headers on the include path (`SDL3/SDL.h`). Set **`SDL3_INCLUDE_DIR`** (parent of `SDL3/SDL.h`) or `scons sdl_include_dir=...` if the default path is wrong; see **`engine/shells/sdl/SConstruct`**.

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
