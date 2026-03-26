# Offline pack validators (`tools/portable_generators`)

Scripts that mirror portable C loaders for **structural** checks on ROM slices or standalone `.bin` packs. They do **not** prove pointer/token closure at runtime.

**Wild encounters (WINF):** emit with **`build_wild_encounter_family_pack.py`**, then validate with **`validate_wild_encounter_family_pack.py`** (standalone **`.bin`**: **`--file-offset-base 0`** on the builder; full **`.gba`**: embed at **`P`** and pass **`--file-offset-base P`** to the builder, **`--pack-offset P`** to the validator).

**Map layout metatile + dimensions + tileset indices:** **Preferred smoke / embed prep:** **`build_map_layout_rom_companion_bundle.py <repo>`** — runs the three per-pack builders, writes **`build/offline_map_layout_rom_companion_bundle.bin`** + **`.manifest.txt`** (offsets/sizes + **`METATILE_PACK_BASE_OFFSET`**), and validates once with **`validate_map_layout_rom_packs.py`**. **`layout_rom_companion_prepare_test_rom.py`** chains that builder (with **`--metatile-pack-base-offset P`**), **`layout_rom_companion_splice_bundle.py`**, and **`layout_rom_companion_emit_embed_env.py`** for a **single-command** test ROM + env output. **`layout_rom_companion_splice_bundle.py`** alone copies a host **`.gba`** to an output path and writes the bundle at **`--offset P`** when **`P`** matches the manifest. Ad hoc: emit metatiles with **`build_map_layout_metatiles_rom_pack.py`**, dimensions with **`build_map_layout_dimensions_rom_pack.py`**, tileset indices with **`build_map_layout_tileset_indices_rom_pack.py`**. Stored **`border_off`** / **`map_off`** are **absolute file offsets** into the mapped ROM (not GBA **`0x08……`**); use **`--pack-base-offset P`** (or companion **`--metatile-pack-base-offset P`**) when **regenerating** the metatile slice for placement at **`P`**. **`validate_map_layout_rom_packs.py`**: per-part or combined ROM; **`--layouts-json`** infers NULL slots and tileset table length. Two-part cross-check only: **`append_validate_map_layout_meta_dim_combined.py`**. **Zero-area border:** **`border_words == 0`**, **`border_off`** may equal **`map_off`**.

**Policy constants:** `map_visual_policy_constants.py` (same folder) centralizes **`MAP_BG_*`-aligned** numbers for Direction D validators + block-word metatile mask + **door reserved tail** / max TID; keep in sync with **`include/fieldmap.h`** / **`include/constants/map_grid_block_word.h`**. **`../validate_field_map_logical_to_physical_inc.py`** (repo `tools/`) imports the same module. **`compiled_tileset_table_roles.py`** parses **`gFireredPortableCompiledTilesetTable`** order + **`Tileset::isSecondary`** for role-based tile caps.

| Script | Pack / seam |
|--------|-------------|
| `validate_map_events_directory_pack.py` | `FIRERED_ROM_MAP_EVENTS_DIRECTORY_PACK_OFF` |
| `validate_map_scripts_directory_pack.py` | `FIRERED_ROM_MAP_SCRIPTS_DIRECTORY_PACK_OFF` |
| `validate_map_connections_pack.py` | `FIRERED_ROM_MAP_CONNECTIONS_PACK_OFF` |
| `validate_wild_encounter_family_pack.py` | `FIRERED_ROM_WILD_ENCOUNTER_FAMILY_PACK_OFF` (WINF v1 + GBA pointer closure) |
| `build_map_layout_metatiles_rom_pack.py` | **`FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF`** dense image (directory @ 0 + appended border/map **`u16`** blobs; same slot order as **`generate_portable_map_data.py`**) |
| `build_map_layout_dimensions_rom_pack.py` | **`FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF`** table (**`slot_count × 8`** bytes, **`u16`** w/h/border per row) from **`layouts.json`** |
| `build_map_layout_tileset_indices_rom_pack.py` | **`FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF`** (**`slot_count × 4`** bytes; **`0xFFFF`** = use compiled / NULL slot) |
| `build_map_layout_rom_companion_bundle.py` | Build all three layout companion packs + concatenated bundle + **`.manifest.txt`** + one **`validate_map_layout_rom_packs.py`** run |
| `layout_rom_companion_emit_embed_env.py` | From **`.manifest.txt`** + bundle base offset **P** → **`FIRERED_ROM_MAP_LAYOUT_*_OFF`** snippets + **`rom_blob_inspect.py`** / validate examples |
| `layout_rom_companion_splice_bundle.py` | Copy host ROM → output; overlay bundle at **P** if **`P == METATILE_PACK_BASE_OFFSET`**; optional **`--validate`** |
| `layout_rom_companion_prepare_test_rom.py` | Build bundle at base **P** + splice into **`--output-rom`** + emit env/inspect (delegates to the three tools above); optional **`--validate`** on spliced ROM |
| `layout_rom_companion_audit_rom_placement.py` | Read-only placement check: **`P + TOTAL_BYTES`**, header-region warning, slice stats; optional **`--suggest-tail-padding`** (heuristic) |
| `append_validate_map_layout_meta_dim_combined.py` | Metatile + dimensions two-pack smoke only |
| `validate_map_layout_rom_packs.py` | Metatile + optional dimensions + optional tileset indices; dimensions- or tileset-only blobs supported; **`--layouts-json`** infers compiled tileset count when validating tileset indices; optional **`--validate-block-words`** |
| `validate_map_layout_block_bin.py` | In repo **`tools/`**: single **`.bin`** or **`--layouts-json`** — default **`--cell-width 2`** (**PRET wire V1** `u16`); **`--cell-width 4`** = Phase 3 documented **`u32`** cell (do not use on vanilla **`u16`** tree) |
| `validate_map_layout_metatile_attributes_directory.py` | `FIRERED_ROM_MAP_LAYOUT_METATILE_ATTRIBUTES_DIRECTORY_OFF` (optional **`--pri-attr-bytes`** / **`--sec-attr-bytes`**) |
| `validate_map_layout_metatile_graphics_directory.py` | `FIRERED_ROM_MAP_LAYOUT_METATILE_GRAPHICS_DIRECTORY_OFF` (optional **`--pri-gfx-bytes`** / **`--sec-gfx-bytes`**) |
| `validate_compiled_tileset_asset_profile_directory.py` | `FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF` (8- or 16-byte rows: tiles, palettes, **`metatile_gfx_bytes`**, **`metatile_attr_bytes`**). Optional **`--enforce-role-tile-caps`** (primary **640x32** vs secondary **384x32** per **`headers.h`** + **`compiled_tileset_table_roles.py`**) |
| `validate_compiled_tileset_tiles_directory.py` | `FIRERED_ROM_COMPILED_TILESET_TILES_DIRECTORY_OFF` (optional `--pri-uncomp-bytes` / `--sec-uncomp-bytes` for mod LZ headers). Optional **`--enforce-role-tile-caps`** + **`--compiled-tileset-table-c`** + **`--tileset-headers`**: LZ decompressed size must equal per-slot cap; raw path clamps **`max-blob`** per role |
| `validate_compiled_tileset_palettes_directory.py` | `FIRERED_ROM_COMPILED_TILESET_PALETTES_DIRECTORY_OFF` |
| `validate_field_map_palette_policy.py` | `FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF` (single `u32` row split) |
| `validate_trainer_money_rom_pack.py` | `FIRERED_ROM_TRAINER_MONEY_TABLE_OFF` |
| `validate_species_names_rom_pack.py` | `FIRERED_ROM_SPECIES_NAMES_PACK_OFF` |
| `validate_facility_class_lookups_rom_pack.py` | `FIRERED_ROM_FACILITY_CLASS_LOOKUPS_PACK_OFF` |
| `validate_move_names_rom_pack.py` | `FIRERED_ROM_MOVE_NAMES_PACK_OFF` |
| `validate_battle_moves_rom_table.py` | `FIRERED_ROM_BATTLE_MOVES_TABLE_OFF` |
| `validate_type_effectiveness_rom_table.py` | `FIRERED_ROM_TYPE_EFFECTIVENESS_OFF` |

**Direction B smoke:** `tools/check_direction_b_offline.py` — **`src/data/field_map_logical_to_physical_identity.inc`** present + **`validate_field_map_logical_to_physical_inc.py`**. **`make check-direction-b-offline`** / **`make check-direction-b`**.

**Project C block-word smoke:** `tools/check_project_c_block_word_offline.py` — **`MAPGRID_*`** vs **`MAP_GRID_BLOCK_WORD_*`** vs **`map_visual_policy_constants.py`**. **`make check-project-c-block-word-offline`**.

**Make:** `make check-map-layout-block-bins` runs the layouts.json sweep; `make check-validators` runs **`check-direction-b-offline`**, **`check-project-c-block-word-offline`**, **`check-project-c-phase3-offline`**, and the layout sweep (needs `python` on `PATH`). **`make check-offline-gates`** runs **`tools/run_offline_build_gates.py`** (Python-only full gate list; no agbcc). **`make check-direction-d`** runs **`check-validators`** plus **`python tools/check_direction_d_offline.py`** (no ROM: compiled tileset table present + role caps derivable).

See also: `docs/architecture/README.md`, `docs/rom_blob_inspection_playbook.md`, `docs/portable_rom_map_layout_metatiles.md`, `docs/architecture/project_c_metatile_id_map_format.md`, `docs/architecture/project_c_map_block_word_and_tooling_boundary.md`.

**Runtime limits:** With **`FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF`**, PORTABLE **`LoadTilesetPalette`** uses **`FireredPortableFieldMapBgPalettePrimaryRows()` / `…SecondaryRows()`** (vanilla **7+6** when the pack is absent or the word is **0**). ROM palette **bundles** can still be larger than the active row count via **`FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`**. Metatile **grid id** semantics (**`MAP_GRID_METATILE_*`** / **`NUM_METATILES_*`**) are unchanged by palette row split alone.

**Layout validator note:** Default checks are **structural** (alignment + blob bounds in ROM). Cross-checking `border_words` / `map_words` vs geometry happens only for slots whose **dimensions row is non-zero** (ROM geometry). Slots with **compiled** geometry (zero dimensions row) match runtime behavior but cannot be fully mirrored offline without `gMapLayouts[]`. Tileset indices: use **`--tileset-indices-off`** and **`--compiled-tileset-count`** (from generated `gFireredPortableCompiledTilesetTableCount`); **`--metatiles-off`** can be omitted for tileset-only checks.

**Border `border.bin` vs runtime:** `validate_map_layout_block_bin.py` checks raw words only. In-game **`GetBorderBlockAt`** ORs **`MAPGRID_COLLISION_MASK`** via **`MapGridBlockWordApplyBorderWrapCollision`** (`map_grid_block_word.h`) so out-of-bounds / wrap tiles are impassable regardless of stored collision bits.
