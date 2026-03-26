# ROM-backed map layout metatiles + effective `MapLayout` (portable)

Portable seam for per-layout **`u16` border + map metatile grids**, optional **ROM-driven geometry**, optional **ROM tileset identity** (indices into a compiled pointer table), optional **ROM metatile attributes / metatile graphics per layout slot**, optional **per-compiled-tileset asset profile metadata** (modded tile sheet sizes), optional **ROM tile sheets** (raw or LZ) per compiled table entry, and optional **ROM full BG palette bundles** for the same table.

## Effective `MapLayout` (runtime)

After refresh, each non-NULL `gMapLayouts[i]` has a matching **`s_effective_layout[i]`** (module static storage):

- **`FireredPortableEffectiveMapLayoutForLayoutId(mapLayoutId)`** returns `&s_effective_layout[slot]` (or compiled rodata **before** first successful refresh / on failure paths).
- **`GetMapLayout()`** (overworld, PORTABLE) uses this accessor so **width / height / borderWidth / borderHeight / border / map / primaryTileset / secondaryTileset** can reflect ROM-backed data where packs are active.

**`map_layout_metatiles_access.h`** — `firered_portable_rom_map_layout_metatiles_refresh_after_rom_load()`, `FireredPortableEffectiveMapLayoutForLayoutId`, `FireredMapLayoutMetatile*ForLayoutId`, legacy `MAP_LAYOUT_METATILE_*_PTR(ml)` (resolves `ml` via `gMapLayouts[]` or effective overlay address).

**Compiled tileset table (PORTABLE):** `gFireredPortableCompiledTilesetTable[]` / `gFireredPortableCompiledTilesetTableCount` — stable sorted list of every `gTileset_*` referenced from vanilla `layouts.json` (emitted into `pokefirered_core/generated/.../map_data_portable.c`). ROM packs index into this table.

**Fieldmap** uses **`FireredPortableEffectiveMapLayoutForLayoutId`** for border sizing, `InitMapLayoutData`, and **connection fill** dimensions (with **`FireredRomMapHeaderScalarsEffectiveMapLayoutId`** for wired `mapLayoutId` on neighbors).

**Metatile attributes / behaviors** (`MapGridGetMetatileAttributeAt`, wild encounters, cut/surf checks, etc.) use the save-backed effective **`MapLayout`** and, on PORTABLE, optional **ROM-backed `u32` attribute tables** per layout slot when **`FIRERED_ROM_MAP_LAYOUT_METATILE_ATTRIBUTES_DIRECTORY_OFF`** is active (see below). Otherwise they use compiled **`Tileset::metatileAttributes`** for the effective primary/secondary tileset pointers. Non-PORTABLE builds keep the classic **`gMapHeader.mapLayout`** + compiled tables only.

## Metatile attributes directory (optional, PORTABLE)

**Env:** `FIRERED_ROM_MAP_LAYOUT_METATILE_ATTRIBUTES_DIRECTORY_OFF` (hex file offset to directory base), profile id **`__firered_builtin_map_layout_metatile_attributes_profile_none__`**.

**Rows:** one per **`gMapLayouts` slot** (`0 .. gMapLayoutsSlotCount - 1`), **8 bytes** little-endian:

| Offset | Size | Field |
|--------|------|--------|
| 0 | **4** | **`primary_attributes_off`** — absolute ROM file offset to **`u32[NUM_METATILES_IN_PRIMARY]`** for this slot’s **effective primary** tileset, or **0** to use compiled |
| 4 | **4** | **`secondary_attributes_off`** — absolute ROM file offset to **`u32[NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY]`** for the **effective secondary** tileset, or **0** for compiled |

**Activation / validation (runtime):** Parsed after the optional **tileset-indices** directory so **effective** primary/secondary `Tileset *` (ROM index overrides or compiled) determine expected **attribute table byte spans**: vanilla **`NUM_METATILES_* × 4`** per role, unless **`FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`** supplies non-zero **`metatile_attr_bytes`** for that compiled tileset index (**extension-only**: non-zero must be ≥ vanilla for that tileset and multiple of **4**). Offsets must be **4-byte aligned** and blobs must fit in the mapped ROM. **`NULL` layout slots** must use **0 / 0** rows. Any failure **clears** the whole attribute overlay (compiled fallback only). Independent of the metatile grid directory: you can enable **attributes-only** with this pack alone.

**API:** `FireredMapLayoutMetatileAttributesPtrForPrimary/Secondary`, **`FireredRomMapLayoutMetatileAttributesRomActive()`** (`map_layout_metatiles_access.h`).

Offline (structural): **`tools/portable_generators/validate_map_layout_metatile_attributes_directory.py`** — optional **`--pri-attr-bytes`** / **`--sec-attr-bytes`** when profile extends attribute blobs uniformly (`tools/docs/README_validators.md`).

## Metatile graphics directory (optional, PORTABLE)

**Env:** `FIRERED_ROM_MAP_LAYOUT_METATILE_GRAPHICS_DIRECTORY_OFF`, profile **`__firered_builtin_map_layout_metatile_graphics_profile_none__`**.

**Rows:** same slotting as attributes — **8 bytes** per layout slot:

| Offset | Size | Field |
|--------|------|--------|
| 0 | **4** | **`primary_metatiles_off`** — ROM offset to **`u16[NUM_METATILES_IN_PRIMARY × NUM_TILES_PER_METATILE]`** (same wire layout as **`Tileset::metatiles`** for the primary half), or **0** for compiled |
| 4 | **4** | **`secondary_metatiles_off`** — ROM offset to **`u16[(NUM_METATILES_TOTAL − NUM_METATILES_IN_PRIMARY) × NUM_TILES_PER_METATILE]`**, or **0** for compiled |

**Sizes (vanilla):** primary **10240** bytes, secondary **6144** bytes. With **`FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`**, non-zero **`metatile_gfx_bytes`** per compiled tileset index sets the expected span for that tileset’s role when validating ROM blobs (**extension-only**, multiple of **`NUM_TILES_PER_METATILE × sizeof(u16)`**). Offsets must be **2-byte aligned**. **`NULL` layout slots** → **0 / 0**. Any failure clears the whole graphics overlay.

**Consumers (PORTABLE):** `DrawMetatileAt` / shop map preview / Teachy TV use **`FireredMapLayoutMetatileGraphicsPtrForPrimary/Secondary`**. **`Tileset::tiles`** for VRAM can be overridden per compiled-table entry via **`FIRERED_ROM_COMPILED_TILESET_TILES_DIRECTORY_OFF`** (below). Map **BG** palette loads (`LoadMapTilesetPalettes` → **`LoadTilesetPalette`**) can use ROM bundles via **`FIRERED_ROM_COMPILED_TILESET_PALETTES_DIRECTORY_OFF`** (below). Per-row palette pointers (e.g. Teachy TV) still use compiled **`Tileset::palettes`** unless extended later.

Offline: **`tools/portable_generators/validate_map_layout_metatile_graphics_directory.py`** — optional **`--pri-gfx-bytes`** / **`--sec-gfx-bytes`** for uniform profile-extended sizes.

## Compiled tileset asset profile (optional, PORTABLE)

**Purpose:** **Asset/profile layer** for hacks that change **tile sheet sizes**, **palette bundle spans**, and **per-compiled-tileset metatile graphics/attribute table spans** used when validating **ROM metatile attribute/graphics blobs** (per layout slot, resolved through effective tileset pointers). Still drives **ROM tile** validation, **`FireredPortableCompiledTilesetVramTileBytesForLoad`**, and **`FireredPortableCompiledTilesetPaletteRowPtr`** bounds. **`LoadTilesetPalette`** primary/secondary **DMA sizes** follow the active **field map palette policy** (vanilla **`NUM_PALS_IN_PRIMARY`** / **`NUM_PALS_TOTAL − NUM_PALS_IN_PRIMARY`** unless **`FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF`** overrides the row split; see below). **Map grid metatile id decoding** (**`NUM_METATILES_IN_PRIMARY`** split, **1024** ceiling in several paths) is unchanged; hacks with more metatiles than the grid encoding allows need a **map format / VRAM** program, not this profile alone.

**Env:** `FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`, profile **`__firered_builtin_compiled_tileset_asset_profile_none__`**.

**Rows:** one per **`gFireredPortableCompiledTilesetTable[]` index** (same count as **`gFireredPortableCompiledTilesetTableCount`**). **Legacy wire: 8 bytes** (first two fields only). **Extended wire: 16 bytes** little-endian (if the mapped ROM provides **`16 × tableCount`** bytes from the profile base; otherwise the loader accepts **`8 × tableCount`** and treats metatile fields as **0**):

| Offset | Size | Field |
|--------|------|--------|
| 0 | **4** | **`tile_uncomp_bytes`** — expected **decompressed** tile sheet size in bytes for this table entry’s tileset role, or **0** for vanilla primary/secondary tile counts × **32** |
| 4 | **4** | **`palette_bundle_bytes`** — **0** for vanilla **416**-byte ROM palette blobs; non-zero: **≥ 416**, **multiple of 32**, **≤ 8192** |
| 8 | **4** | **`metatile_gfx_bytes`** — **0** for vanilla metatile graphics table size for this tileset’s role; non-zero: **extension-only** (≥ vanilla, multiple of **`NUM_TILES_PER_METATILE × 2`**, capped at **2048** metatiles worth) |
| 12 | **4** | **`metatile_attr_bytes`** — **0** for vanilla **`NUM_METATILES_* × 4`** for this role; non-zero: **extension-only** (≥ vanilla, multiple of **4**, capped at **2048** metatiles worth) |

**Validation (runtime):** **`NULL` table slots** must use **all-zero** rows (for extended rows, all four **`u32`**). Parse failure clears the profile.

**Order:** Loaded **before** compiled tileset **tiles**, **palette** directories, and **before** layout-slot **metatile attribute/graphics** directories (refresh order: asset profile → … → tileset indices → metatile attrs/gfx).

**API:** **`FireredRomCompiledTilesetAssetProfileRomActive()`**, **`FireredPortableCompiledTilesetVramTileBytesForLoad`** (`map_layout_metatiles_access.h`).

**Desync warning:** **Map metatile id range** / tile-index math in **`field_camera`** still assumes vanilla **640 / 1024** encoding (Project C). **Policy:** **`MAP_BG_*`**, **`MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES`**, **`MAP_BG_FIELD_TILESET_MAX_TILE_INDEX`**, char-block **linear** formula in **`fieldmap.h`** (≤ **1024** tile numbers, ≤ **2×16KiB** span from one char base). **PORTABLE:** upload / BG agreement / **DMA** bounds as before. **>1024** or non-linear char layout → **layout redesign**, not ROM profile alone.

**Larger redesign:** See **`docs/architecture/project_b_field_tileset_redesign.md`** for ranked options (strict caps, logical indirection, multi–char-base, renderer virtual plane), **coupling table**, and phased work.

**Direction D tooling (MAP_BG caps, charter closed):** `validate_compiled_tileset_asset_profile_directory.py` (`tile_uncomp_bytes` ≤ **32768**; optional **`--enforce-role-tile-caps`**); `validate_compiled_tileset_tiles_directory.py` (pri/sec/`max-blob` ≤ **32768**; optional **`--enforce-role-tile-caps`** for strict per-slot LZ sizes); `validate_map_layout_metatile_graphics_directory.py` and `validate_map_layout_metatile_attributes_directory.py` default to **vanilla 640/384** metatile blob sizes — use **`--allow-exceed-map-bg-metatile-gfx`** / **`--allow-exceed-map-bg-metatile-attr`** for extended profile packs. Offline smoke: **`tools/check_direction_d_offline.py`** / **`make check-direction-d`**.

**Inventory:** `docs/architecture/project_b_field_tilemap_tile_index_inventory.md`.

Offline: **`tools/portable_generators/validate_compiled_tileset_asset_profile_directory.py`** (detects **8-** vs **16-byte** stride from ROM tail length).

## Compiled tileset tile sheets directory (optional, PORTABLE)

**Env:** `FIRERED_ROM_COMPILED_TILESET_TILES_DIRECTORY_OFF`, profile **`__firered_builtin_compiled_tileset_tiles_profile_none__`**.

**Rows:** one **`u32` file offset per index** in **`gFireredPortableCompiledTilesetTable[]`** (same count as **`gFireredPortableCompiledTilesetTableCount`**). **`0`** → use compiled **`Tileset::tiles`**.

**Wire:** contiguous **`u32[]`**, **`4 × tableCount`** bytes. Each non-zero offset points at tile data in one of two forms (chosen from **`Tileset::isCompressed`** at refresh). **Expected decompressed byte size** per index is vanilla primary/secondary unless **`FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`** sets **`tile_uncomp_bytes`** for that index.

- **`isCompressed == FALSE`:** **raw** uncompressed 4bpp tile bytes spanning exactly the expected size.
- **`isCompressed != FALSE`:** **GBA LZ77** stream (same format as compiled **`Tileset::tiles`**): first byte **`0x10`**, next three bytes little-endian **decompressed size** must equal the expected size.

Offsets must be **4-byte aligned**; raw blobs must be fully contained in the mapped ROM; LZ streams must have a valid header and at least **4** bytes available (compressed length is not encoded separately — packs must place blobs so the decompressor does not read past the ROM image).

**Consumers:** **`CopyMapTilesetsToVram`** / **`CopyTilesetToVram(UsingHeap)`** (`fieldmap.c`, including **`DecompressAndCopyTileDataToVram2`** / **`DecompressAndLoadBgGfxUsingHeap2`**) and **Teachy TV** **`LZDecompressWram`** use **`FireredPortableCompiledTilesetTilesForLoad`**.

**API:** **`FireredRomCompiledTilesetTilesRomActive()`**, **`FireredPortableCompiledTilesetTilesForLoad`** (`map_layout_metatiles_access.h`).

Offline: **`tools/portable_generators/validate_compiled_tileset_tiles_directory.py`** — raw rows use **`--max-blob`** (default primary decompressed size); LZ rows accept headers whose declared decompressed size is **`--pri-uncomp-bytes`** or **`--sec-uncomp-bytes`** (vanilla defaults **20480** / **12288**). For packs that use **`FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`**, validate that directory with **`validate_compiled_tileset_asset_profile_directory.py`** and align tile/LZ expectations per row. Runtime **`fieldmap.c`** / **Teachy TV** staging use **`FireredPortableCompiledTilesetVramTileBytesForLoad`** (profile override or vanilla tile counts × 32).

## Compiled tileset palette bundles directory (optional, PORTABLE)

**Env:** `FIRERED_ROM_COMPILED_TILESET_PALETTES_DIRECTORY_OFF`, profile **`__firered_builtin_compiled_tileset_palettes_profile_none__`**.

**Rows:** one **`u32` file offset per index** in **`gFireredPortableCompiledTilesetTable[]`**. **`0`** → use compiled **`Tileset::palettes`** for **`LoadTilesetPalette`**.

**Wire:** each non-zero offset points at a contiguous **`u16`-ish row-major palette bundle**: **vanilla** size is **`NUM_PALS_TOTAL × PLTT_SIZE_4BPP`** (**416**), i.e. **`u16[13][16]`**, same layout as compiled **`palettes`**. If **`FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`** sets non-zero **`palette_bundle_bytes`** for that index, the blob must be exactly that many bytes (still a multiple of **32**, minimum **416**). **`FireredPortableCompiledTilesetPaletteRowPtr`** allows **`row < bundle_bytes / 32`**. Runtime **`LoadTilesetPalette`** loads **`FireredPortableFieldMapBgPalettePrimaryRows()`** rows from the primary tileset (after the usual black slot-0 entry) and **`FireredPortableFieldMapBgPaletteSecondaryRows()`** rows from bundle row **`FireredPortableFieldMapBgPalettePrimaryRows()`** on the secondary tileset (see **`FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF`**); trailing bundle rows beyond the active policy are optional.

**Validation:** offsets must be **2-byte aligned**; blob must fit in the mapped ROM. Entries whose tileset is not in the standard primary/secondary **`LoadPalette`** path (**`LoadCompressedPalette`**) must use **`0`** (non-zero fails the whole pack). **`isCompressed` tilesets** may still use ROM palettes when they follow the normal **`LoadPalette`** branches.

**Consumers:** **`LoadTilesetPalette`** (`fieldmap.c`, PORTABLE) via **`FireredPortableCompiledTilesetPaletteRowPtr`**; **Teachy TV** via **`FireredPortableCompiledTilesetPaletteRowPtr`** for referenced rows. **`FireredPortableCompiledTilesetPalettePtrForLoad`** remains a thin wrapper for primary/secondary base rows.

**API:** **`FireredRomCompiledTilesetPalettesRomActive()`**, **`FireredPortableCompiledTilesetPaletteRowPtr`**, **`FireredPortableCompiledTilesetPalettePtrForLoad`** (`map_layout_metatiles_access.h`).

Offline: **`tools/portable_generators/validate_compiled_tileset_palettes_directory.py`** — default **`--blob-bytes`** **416**; for profile-extended bundles use the same per-index size as **`palette_bundle_bytes`** (uniform hacks can pass one larger **`--blob-bytes`** when every non-zero row matches).

## Field map BG palette row split (optional, PORTABLE)

**Env:** `FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF` — **4-byte aligned** absolute file offset to one little-endian **`u32`**.

**Profile id:** **`__firered_builtin_field_map_palette_policy_none__`** (same env/profile resolution pattern as other `FIRERED_ROM_*` packs).

**Wire word:**

- **`0`**: use vanilla split (**`NUM_PALS_IN_PRIMARY`** primary rows, **`NUM_PALS_TOTAL − NUM_PALS_IN_PRIMARY`** secondary rows). Does **not** set **`FireredRomFieldMapPalettePolicyRomActive()`**.
- **Non-zero**: low byte = **primary row count**, next byte = **secondary row count**, upper **16** bits must be **0**. **`primary + secondary`** must equal **`NUM_PALS_TOTAL`** (**13**); each count must be **≥ 1**. On success, **`FireredRomFieldMapPalettePolicyRomActive()`** is **TRUE**.

**Refresh order:** Parsed during **`firered_portable_rom_map_layout_metatiles_refresh_after_rom_load`** as soon as the ROM image is valid, **before** optional map-layout directory packs (so the split applies even when no metatile / compiled-tileset directories are present).

**Consumers:** **`LoadMapTilesetPalettes`** / **`LoadTilesetPalette`** (`src_transformed/fieldmap.c`, PORTABLE), **`FireredPortableCompiledTilesetPalettePtrForLoad`**, **Teachy TV** primary vs secondary palette row selection (`src_transformed/teachy_tv.c`), **`LoadMapFromCameraTransition`** weather gamma on **secondary** map slots (`overworld.c`: slots **`FireredPortableFieldMapBgPalettePrimaryRows()` .. `NUM_PALS_TOTAL-1`**), **Pokémon League lighting** (`field_specials.c`: loads/tints the **first secondary** slot and matching **`BlendPalettes`** bit). Non-PORTABLE builds resolve the accessor macros to vanilla **`NUM_PALS_IN_PRIMARY`**.

**API:** **`FireredPortableFieldMapBgPalettePrimaryRows()`**, **`FireredPortableFieldMapBgPaletteSecondaryRows()`**, **`FireredRomFieldMapPalettePolicyRomActive()`** (`map_layout_metatiles_access.h`).

**Caveat:** Map metatile **palette indices** in map data still use vanilla **0..12** semantics; changing the split only changes **which tileset bundle rows** fill BG slots **0..12**. Hacks that need different **metatile → palette index** encoding remain **Project C** (map format).

**Project A closure:** For **row split** inside **`NUM_PALS_TOTAL`**, the remaining field-map palette seams are addressed in **`fieldmap`**, **Teachy TV**, **overworld** (secondary gamma), **League lighting**, and this ROM pack. Unifying **BG slots 13–15**, **OBJ** regions, and screen-specific **`BG_PLTT_ID(n)`** choices is **outside** this policy (cross-subsystem **palette RAM contract**).

### Natural boundary: `LoadCompressedPalette` tileset branch (field BG)

`LoadTilesetPalette` has a third branch: **`LoadCompressedPalette((const u32 *)tileset->palettes, …)`** when **`isSecondary`** is neither **`FALSE`** nor **`TRUE`**. **Vanilla FireRed map tilesets** only use **`FALSE` / `TRUE`**, so this path is **unused** for live overworld / layout-driven tileset loads. The ROM-backed palette bundle + **`FireredPortableCompiledTilesetPaletteRowPtr`** already cover every **real** map BG palette upload.

A future **compressed-palette ROM seam** for that branch would need a **different wire shape** (single LZ stream pointer, not the **`u16[13][16]`** row bundle) and a way to key it off **`gFireredPortableCompiledTilesetTable[]`** for hypothetical tilesets — **out of scope** until a project actually ships non-boolean **`isSecondary`** map tilesets or defines a pack format. **`LoadCompressedPalette`** for field tilesets stays **compiled-only** by design at this frontier.

**ROM tile/LZ validation** and **PORTABLE VRAM/staging tile byte counts** use **`tile_uncomp_bytes`** when present. **ROM palette blob size** and **`PaletteRowPtr`** bounds use **`palette_bundle_bytes`** when present.

## Metatile directory (ROM file offsets)

After `engine_memory_init`, the mapped ROM image starts at **`ENGINE_ROM_ADDR`**.

For **`i = 0 .. gMapLayoutsSlotCount - 1`**, metatile row **`i`** is **16 bytes**, little-endian **`u32`**:

| Offset in row | Field            | Meaning                                      |
|---------------|------------------|----------------------------------------------|
| 0             | `border_off`     | Absolute file offset of border `u16[]`       |
| 4             | `map_off`        | Absolute file offset of map `u16[]`          |
| 8             | `border_words`   | Word count = `borderWidth * borderHeight` (may be **0**) |
| 12            | `map_words`      | Word count = `width * height`                |

- **`gMapLayouts[i] == NULL`**: all four words **must be 0**.
- **`gMapLayouts[i] != NULL`**: `border_words` / `map_words` must equal the **effective** width/height and border sizes for that slot (compiled `MapLayout`, or ROM dimensions row when present — see below). **Zero-area border** (`borderWidth == 0` or `borderHeight == 0`): `border_words == 0`; `border_off` may equal `map_off` and no border payload is required (builder emits no border bytes). Out-of-bounds map queries use **`GetBorderBlockAt`** in `fieldmap.c`, which returns an impassable wrapped word without indexing a border grid when width or height is 0.

**All-or-nothing:** if any row fails validation, the metatile seam **deactivates** and runtime **falls back** to compiled layouts only (no mixed ROM state).

**Block word layout (Project C):** each element of those `u16[]` blobs is one **map block word** (10-bit metatile id, collision, elevation — `MAPGRID_*` in `global.fieldmap.h`). Wire id **`MAP_GRID_BLOCK_WORD_LAYOUT_VANILLA`** and helpers: `include/constants/map_grid_block_word.h`, `include/map_grid_block_word.h`. Offline: `tools/validate_map_layout_block_bin.py` / `make check-map-layout-block-bins`, and `tools/portable_generators/validate_map_layout_rom_packs.py --validate-block-words`. Boundary doc: `docs/architecture/project_c_map_block_word_and_tooling_boundary.md`.

## Optional dimensions directory

**Env:** **`FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF`** — hex offset to a parallel table of **`gMapLayoutsSlotCount`** rows, **8 bytes** each, little-endian **`u16`**:

| Offset | Field          | Meaning        |
|--------|----------------|----------------|
| 0      | `width`        | Map width      |
| 2      | `height`       | Map height     |
| 4      | `borderWidth`  | Must be ≤ 255  |
| 6      | `borderHeight` | Must be ≤ 255  |

- **`gMapLayouts[i] == NULL`**: row must be **all zero**.
- **Non-NULL layout**: **all zero** = use compiled geometry for that slot; **non-zero** = ROM geometry for that slot. **`width`** and **`height`** must be non-zero when any dimension field is non-zero; **`borderWidth`** / **`borderHeight`** may be **0** (zero-area border). Each border field must still be **≤ 255** when the row is used.

**Profile id:** **`__firered_builtin_map_layout_dimensions_profile_none__`**.

If the dimensions blob fails structural validation, it is **ignored** (compiled geometry used); the metatile directory is still evaluated.

ROM geometry must **match** the metatile row word counts (`map_words == width*height`, `border_words == borderWidth*borderHeight`).

## Optional tileset-indices directory

**Env:** **`FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF`** — hex offset to **`gMapLayoutsSlotCount`** rows, **4 bytes** each, little-endian **`u16`**:

| Offset | Field | Meaning |
|--------|-------|---------|
| 0 | `primary_index` | Index into **`gFireredPortableCompiledTilesetTable[]`**, or **0xFFFF** to keep compiled **`MapLayout::primaryTileset`** |
| 2 | `secondary_index` | Same for secondary |

**`gMapLayouts[i] == NULL`:** both words **0xFFFF** (required).

**Table order:** indices refer to **`gFireredPortableCompiledTilesetTable`** as emitted from **`generate_portable_map_data.py`**: every distinct **`primary_tileset`** / **`secondary_tileset`** string from layouts with an **`id`**, sorted lexicographically, **excluding** the literal **`"NULL"`** symbol (that role uses **0xFFFF** in the directory instead).

**Profile id:** **`__firered_builtin_map_layout_tileset_indices_profile_none__`**.

**Emit:** `tools/portable_generators/build_map_layout_tileset_indices_rom_pack.py <repo> <out.bin>`. **Validate:** `validate_map_layout_rom_packs.py … --tileset-indices-off 0x0 --layouts-json data/layouts/layouts.json` (**`--compiled-tileset-count`** optional; inferred from **`layouts.json`** and must match if both are passed).

## Env / profile (metatile directory)

- **Env:** `FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF`
- **Profile:** **`__firered_builtin_map_layout_metatiles_profile_none__`**

## Offline structural check

`tools/portable_generators/validate_map_layout_rom_packs.py` validates directory layout, alignment, and ROM blob bounds. With **`--dimensions-off`**, it also validates the dimensions table and cross-checks metatile word counts **only** for slots that use a **non-zero** dimensions row (ROM geometry). Slots that keep **compiled** geometry (zero dimensions row) are not fully checkable offline without the built `MapLayout` table — see `tools/docs/README_validators.md`.

**Emit a standalone pack (same slotting as `generate_portable_map_data.py`):** `tools/portable_generators/build_map_layout_metatiles_rom_pack.py <repo> <out.bin>` reads **`data/layouts/layouts.json`** and layout **`border.bin`** / **`map.bin`** paths; rejects on-disk word counts that disagree with JSON **`width` × `height`** and **`border_width` × `border_height`**. Then validate, for example:

```bash
python tools/portable_generators/validate_map_layout_rom_packs.py build/offline_map_layout_metatiles_pack.bin \
  --metatiles-off 0x0 --layouts-json data/layouts/layouts.json --validate-block-words
```

**Dimensions directory image:** `tools/portable_generators/build_map_layout_dimensions_rom_pack.py <repo> <out.bin>` emits **`gMapLayoutsSlotCount × 8`** bytes (**`u16`** width, height, border width, height per slot). Validate standalone with **`--dimensions-off 0x0`** (no metatile blob required). To cross-check metatile + dimensions only (without tileset indices), concatenate **metatile image first**, then dimensions, and pass **`--metatiles-off 0x0`** plus **`--dimensions-off`** = **hex byte length of the metatile file** — see **`tools/portable_generators/append_validate_map_layout_meta_dim_combined.py`**. For all three parts, use **`build_map_layout_rom_companion_bundle.py`** (offline gates / CI).

**Tileset-indices directory:** `build_map_layout_tileset_indices_rom_pack.py` + **`validate_map_layout_rom_packs.py --tileset-indices-off`** (see § Optional tileset-indices directory).

**One-shot companion bundle (recommended for ROM embed / smoke):** `tools/portable_generators/build_map_layout_rom_companion_bundle.py <repo>` builds the three per-part binaries under **`build/`**, concatenates **`[metatile][dimensions][tileset indices]`**, writes **`build/offline_map_layout_rom_companion_bundle.bin`** plus **`.manifest.txt`** (hex offsets + sizes relative to the bundle start, plus **`METATILE_PACK_BASE_OFFSET`** matching **`--metatile-pack-base-offset`**), and runs **`validate_map_layout_rom_packs.py`** once with **`--metatiles-off 0x0`**, **`--dimensions-off`**, **`--tileset-indices-off`**, **`--layouts-json`**, **`--validate-block-words`**. For host placement at file offset **P ≠ 0**, rebuild with **`--metatile-pack-base-offset P`** so metatile directory **`border_off`** / **`map_off`** are absolute in the host image; **`METATILE_PACK_BASE_OFFSET`** in the manifest must equal **P** before splicing.

**Splice a test copy (no in-place ROM writes):** `tools/portable_generators/layout_rom_companion_splice_bundle.py --input base.gba --output test.gba --offset P --bundle build/offline_map_layout_rom_companion_bundle.bin [--validate]` copies the host image, overwrites **`[P, P+TOTAL_BYTES)`** with the bundle, refuses unless **`P == METATILE_PACK_BASE_OFFSET`**, then runs **`layout_rom_companion_emit_embed_env.py`** (unless **`--no-emit-env`**) and optionally **`validate_map_layout_rom_packs.py`** on **`test.gba`**.

**One-shot test ROM (build + splice + env):** `tools/portable_generators/layout_rom_companion_prepare_test_rom.py --input-rom base.gba --output-rom test.gba --offset 0xP [--validate]` shells out to the bundle builder (with **`--metatile-pack-base-offset P`**), **`layout_rom_companion_splice_bundle.py`** (with **`--no-emit-env`**), then **`layout_rom_companion_emit_embed_env.py`** so **P** is consistent end-to-end. **`--input-rom`** and **`--output-rom`** must differ.

```bash
python tools/portable_generators/layout_rom_companion_prepare_test_rom.py \
  --input-rom path/to/pokefirered.gba --output-rom path/to/pokefirered_layout_test.gba \
  --offset 0x100000 --validate
```

**Choosing `P` on a real `.gba` (conservative):** The companion bundle is on the order of **~500 KiB**; the exact size is **`TOTAL_BYTES`** in the **`.manifest.txt`** next to the bundle. Nothing in-repo can prove a byte range is unused by the game — **P** must come from your **linker map**, **ROM expansion / relocation plan**, or another tool you trust.

Read-only helpers:

1. **`tools/portable_generators/layout_rom_companion_audit_rom_placement.py --rom X.gba --offset 0xP --manifest …`** (or **`--bundle`** / **`--byte-count`**) checks **`P + TOTAL_BYTES ≤ ROM size`**, warns if **`P < 0x200`** (vector/header region), and prints **slice statistics** (distinct bytes, **`0x00` / `0xFF`** fractions). Uniform **`0xFF`** is *not* proof of safety; non-uniform data is a strong hint you should not overwrite without re-checking.
2. **`--suggest-tail-padding`** (optional **`--fill-byte`**, **`--align`**) reports a candidate **`P`** only when there is a long **trailing** constant fill from EOF (often **0xFF** cartridge padding). This is a **heuristic** for hacks that still have a padded tail; many expanded or heavily patched ROMs may not — then you must pick **`P`** elsewhere.
3. **`tools/rom_blob_inspect.py`** on **`[P, P+n)`** for a small **`n`** (see **`docs/rom_blob_inspection_playbook.md`**) is the manual eyeball step before you splice a **copy**.

Workflow: decide **`P`** from project tooling → **audit** → **hexdump slice** → **`layout_rom_companion_prepare_test_rom.py`** (or build + splice) on a **copy** → runtime env from **`layout_rom_companion_emit_embed_env.py`**.

**Example (manual steps, nonzero P):**

```bash
python tools/portable_generators/build_map_layout_rom_companion_bundle.py . build/my_layout_bundle.bin \
  --metatile-pack-base-offset 0x100000
python tools/portable_generators/layout_rom_companion_splice_bundle.py \
  --input path/to/pokefirered.gba --output path/to/pokefirered_layout_test.gba \
  --offset 0x100000 --bundle build/my_layout_bundle.bin --validate
```

**Runtime / inspect ergonomics:** `tools/portable_generators/layout_rom_companion_emit_embed_env.py <bundle>.manifest.txt -b <P> [--rom path.gba]` prints **`FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF`**, **`FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF`**, and **`FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF`** as shell exports (and PowerShell **`$env:`** lines), plus suggested **`tools/rom_blob_inspect.py`** and **`validate_map_layout_rom_packs.py`** commands using **absolute** file offsets (**`P` + manifest relative offset**). When the manifest’s **`METATILE_PACK_BASE_OFFSET`** matches **P**, no metatile pointer warning is emitted; legacy manifests without that key still warn for **`-b` ≠ 0**.

**`--layouts-json`** sets **`gMapLayoutsSlotCount`** from **`len(layouts)`** and treats JSON rows **without** an **`id`** as **NULL** slots (unless **`--null-slots`** is passed explicitly). **`tools/run_offline_build_gates.py`** exercises **`layout_rom_companion_prepare_test_rom.py`** on a synthetic host image (**`--offset 0`**, **`--validate`**). **`tools/verify_portable_default.sh`**, **`.ps1`**, and **`.github/workflows/portable_host.yml`** / **`build.yml`** still run **`build_map_layout_rom_companion_bundle.py`** plus **`layout_rom_companion_emit_embed_env.py … -b 0`** after the layout **`.bin`** sweep (no ROM splice in those paths).

## Real-ROM validation (manual; not run in CI)

**Repository status:** This tree does **not** ship a **`.gba`**. There is **no** checked-in, authoritative **file offset `P`** documented as “guaranteed unused” on retail **BPRE** (16 MiB **`0x1000000`**) or any specific hack. Therefore **automated** “first real-ROM” SDL proof is **not** recorded here—only the **recipe** and **blockers**.

**Trustworthy `P` (required before splicing):** Obtain **`P`** from **your** linker map, ROM expansion layout, or another source you treat as ground truth. Optionally cross-check with **`layout_rom_companion_audit_rom_placement.py`** and **`rom_blob_inspect.py`**; **`--suggest-tail-padding`** remains a **heuristic**, not approval.

**Offline prep (copy-only):** from repo root, with legal ROM **`base.gba`** and chosen **`P`**:

```bash
python tools/portable_generators/layout_rom_companion_audit_rom_placement.py \
  --rom base.gba --offset 0xP --bundle build/offline_map_layout_rom_companion_bundle.bin
python tools/portable_generators/layout_rom_companion_prepare_test_rom.py \
  --input-rom base.gba --output-rom layout_test.gba --offset 0xP --validate --repo-root .
```

The second command prints **`FIRERED_ROM_MAP_LAYOUT_*_OFF`** exports; apply them in the same shell before launching SDL (see **`engine/shells/sdl/README.md`**).

**SDL runtime:** Build **`engine/shells/sdl`** (e.g. **`build/decomp_engine_sdl.exe`** on Windows), then run against **`layout_test.gba`** with the three env vars set. Narrow manual checks: title → load save or new game → overworld **map/tiles** look sane (no instant crash, no obvious metatile garbage).

**If something fails:** (1) **`P + TOTAL_BYTES`** vs ROM size and overlap with game data; (2) **`METATILE_PACK_BASE_OFFSET == P`** and **`prepare_test_rom`** completed without splice errors; (3) seam / validation logs vs unrelated SDL or GPU issues.

## Activation

`firered_portable_rom_map_layout_metatiles_refresh_after_rom_load()` runs from **`engine_backend_init`**.  
Indexed by slot **`i` = `mapLayoutId - 1`**.

**Cross-table:** **`FIRERED_ROM_MAP_HEADER_SCALARS_PACK`** + effective layout id on connections (`docs/portable_rom_map_header_scalars_pack.md`). Query **`FireredRomMapLayoutMetatilesRomActive()`** / **`FireredRomMapLayoutTilesetIndicesRomActive()`**.

**Teachy TV (PORTABLE):** `TeachyTvLoadBg3Map` resolves **Route 1** via **`Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_ROUTE1), …)`** and uses **`FireredPortableEffectiveMapLayoutForLayoutId(mapLayoutId)`** so the BG3 preview follows ROM-backed geometry / metatiles / tileset indices when active (fallback: compiled **`Route1_Layout`**). Tile bytes use **`FireredPortableCompiledTilesetTilesForLoad`**; palette rows use **`FireredPortableCompiledTilesetPaletteRowPtr`** so ROM palette bundles apply to the preview.

**Other consumers (PORTABLE):** **`fieldmap.c`** uses **`FireredRomMapHeaderScalarsEffectiveMapLayoutId`** + **`FireredPortableEffectiveMapLayoutForLayoutId`** for **neighbor** maps in connection camera placement (`SetPositionFromConnection`), incoming/outgoing connection hit tests, and (already) connection fill. **`itemfinder.c`** uses the same for **connected-map** hidden-item coordinate math. **`region_map.c`** (via **`region_map_portable.c`**) uses effective layout dimensions for the region-map cursor. Non-transformed **`src/overworld.c`** **`GetMapLayout`** matches **`src_transformed/overworld.c`** when built with **`PORTABLE`**.

## Architectural boundary (map / layout visuals, current stack)

The **ROM-backed + compiled-tileset asset profile** path has absorbed the seam-sized metadata that can stay coherent without redefining **rendering policy** or **map encoding**:

- **Tiles, palettes (blob + row bounds), metatile graphics/attributes (ROM span validation)** are driven or constrained by **`FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`** (with **8- or 16-byte** legacy-compatible rows).
- **Further “one more `u32`”** fields do **not** safely fix what is left; those issues are **architectural** and cross-cut **`fieldmap.c`**, **BG / VRAM**, and **game logic**.

**Explicit remaining frontiers (not another profile row):**

1. **`LoadTilesetPalette` / `LoadMapTilesetPalettes` (`fieldmap.c`)** — DMA **halfword counts** and **BG palette slot layout** still assume vanilla **`NUM_PALS_IN_PRIMARY`** and **`NUM_PALS_TOTAL − NUM_PALS_IN_PRIMARY`**, and hard-wire **`BG_PLTT_ID(0)`** / **`BG_PLTT_ID(NUM_PALS_IN_PRIMARY)`** for the secondary base row index into the bundle. Making the **primary/secondary palette upload split** configurable implies a **palette policy** (how many rows load, where they land in **`gPlttBufferUnfaded`**, tint/quest-log interactions, and any code that assumes **13** rows in fixed slots). That is a **coordinated fieldmap + consumer** change, not a portable-ROM-table tweak.

2. **VRAM / BG char layout** — Larger **tile sheets** or **tile indices** can exceed what the current **BG2** setup and **char base** mapping can represent on GBA-style rules. The portable **SDL** path shares the same **logical** limits until a **renderer / VRAM contract** defines new budgets and possibly **multiple BG** or **dynamic** char allocation.

3. **Map metatile ID semantics** — **`NUM_METATILES_IN_PRIMARY`**, **`NUM_METATILES_TOTAL`**, and the **640 / 384 / 1024** split still govern **attribute lookup**, **camera / DrawMetatile** paths, **shop / Teachy** previews, etc. ROM tables may be **larger** than vanilla, but **map grid values** and **runtime branches** still assume the classic split unless a **map format or engine** project redefines metatile id encoding. **Project C** documents the contract and central interpreters: **`docs/architecture/project_c_metatile_id_map_format.md`**, **`MAP_GRID_METATILE_*`** in **`fieldmap.h`**. Map **block word** / tooling frontier: **`docs/architecture/project_c_map_block_word_and_tooling_boundary.md`**.

**Conclusion:** Under the **present** architecture, the **map/layout visual content** ROM frontier for **metadata-sized** seams is **effectively complete**. Next work should be scoped as **palette policy**, **VRAM/layout**, and/or **metatile id / map encoding** programs—not incremental rows on the same profile table. Cross-lane research and ranking: **`docs/architecture/map_visual_three_frontiers.md`** (first bounded palette-policy hook: **`MAP_BG_*`** in `fieldmap.h`).

## Direction B — field BG logical→physical table (workflow)

When changing **`src/data/field_map_logical_to_physical_identity.inc`** / **`gFieldMapLogicalToPhysicalTileId`**, run **`tools/validate_field_map_logical_to_physical_inc.py`** (or **`make check-field-map-logical-physical-table`**) or **`tools/check_direction_b_offline.py`** (**`make check-direction-b-offline`**). The **portable** merge gate **`.github/workflows/portable_host.yml`** and **`tools/verify_portable_default.sh`** / **`.ps1`** run **`check_direction_b_offline.py`** (same validator + **`.inc`** presence). **`--strict-injective`** is for hack-specific pipelines, not the default workflow. Full contract: **`docs/architecture/project_b_field_tileset_redesign.md`** §5.

## Next migration steps (out of scope here)

- ROM-backed **full `struct Tileset` blobs** (not just compiled-table indices)  
- Map headers / `gMapGroups` / events beyond current seams  
- Broader `map_data_portable.c` teardown  
- **Palette upload policy** and **BG/VRAM layout** as first-class design targets (see boundary above)
