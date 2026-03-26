#ifndef GUARD_MAP_LAYOUT_METATILES_ACCESS_H
#define GUARD_MAP_LAYOUT_METATILES_ACCESS_H

#include "global.h"
#include "fieldmap.h"

struct MapLayout;

/*
 * PORTABLE: optional ROM-backed **`u16` border + map grids** (same **block word** layout as vanilla
 * `map.bin` / `VMap` / save `mapView`: `MAP_GRID_BLOCK_WORD_LAYOUT_VANILLA`,
 * `include/constants/map_grid_block_word.h`, pack/unpack `map_grid_block_word.h`).
 * See docs/portable_rom_map_layout_metatiles.md.
 *
 * Primary resolution is by **layout id** (1-based slot index). Runtime keeps
 * `gMapHeader.mapLayoutId` in sync with `gSaveBlock1Ptr->mapLayoutId` (save is
 * authoritative for `GetMapLayout`) when binding `gMapHeader.mapLayout`. Pointer-based
 * helpers scan `gMapLayouts[]` for backwards compatibility (e.g. teachy_tv).
 */
#ifdef PORTABLE
void firered_portable_rom_map_layout_metatiles_refresh_after_rom_load(void);

/* Indices for FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF (0xFFFF = use compiled layout). */
extern const struct Tileset *const gFireredPortableCompiledTilesetTable[];
extern const u32 gFireredPortableCompiledTilesetTableCount;

bool8 FireredRomMapLayoutMetatilesRomActive(void);
bool8 FireredRomMapLayoutTilesetIndicesRomActive(void);
bool8 FireredRomMapLayoutMetatileAttributesRomActive(void);
bool8 FireredRomMapLayoutMetatileGraphicsRomActive(void);
bool8 FireredRomCompiledTilesetTilesRomActive(void);
bool8 FireredRomCompiledTilesetPalettesRomActive(void);
bool8 FireredRomCompiledTilesetAssetProfileRomActive(void);
/*
 * Optional ROM word at `FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF` selects how many BG palette
 * rows the field map loads from the primary vs secondary tileset bundles (sum must stay
 * `NUM_PALS_TOTAL`). See `docs/portable_rom_map_layout_metatiles.md`.
 */
bool8 FireredRomFieldMapPalettePolicyRomActive(void);
u8 FireredPortableFieldMapBgPalettePrimaryRows(void);
u8 FireredPortableFieldMapBgPaletteSecondaryRows(void);
u32 FireredPortableCompiledTilesetVramTileBytesForLoad(const struct Tileset *tileset, u16 numTilesVanilla);

/*
 * Optional ROM-backed 4bpp tile **source** for entries in
 * `gFireredPortableCompiledTilesetTable[]` (see docs/portable_rom_map_layout_metatiles.md):
 * either **raw** uncompressed tile bytes or **GBA LZ77** (`0x10` header + 24-bit decompressed size).
 * Expected decompressed byte size is vanilla primary/secondary unless overridden by
 * **`FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`** (`tile_uncomp_bytes`; see docs).
 */
const u32 *FireredPortableCompiledTilesetTilesForLoad(const struct Tileset *tileset);

/*
 * Optional ROM-backed **uncompressed** BG palette bundle for the same table (see
 * docs/portable_rom_map_layout_metatiles.md). Default wire size: `NUM_PALS_TOTAL` rows ×
 * `PLTT_SIZE_4BPP` bytes (vanilla layout). `FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF`
 * may set a larger per-index `palette_bundle_bytes` (multiple of `PLTT_SIZE_4BPP`, min vanilla).
 * `LoadCompressedPalette` tilesets must use directory row **0** (non-zero fails the pack).
 */
const u16 *FireredPortableCompiledTilesetPaletteRowPtr(const struct Tileset *tileset, u32 row);
const u16 *FireredPortableCompiledTilesetPalettePtrForLoad(const struct Tileset *tileset);

/*
 * Per-layout-slot optional ROM-backed `u32` metatile attribute tables (see
 * docs/portable_rom_map_layout_metatiles.md). `NULL` / inactive → use compiled
 * `Tileset::metatileAttributes` from the effective layout's tileset pointers.
 */
const u32 *FireredMapLayoutMetatileAttributesPtrForPrimary(const struct MapLayout *layout);
const u32 *FireredMapLayoutMetatileAttributesPtrForSecondary(const struct MapLayout *layout);

/*
 * Per-layout-slot optional ROM-backed metatile **graphics** (`u16` tilemaps per metatile, 8
 * halfwords each — same layout as `Tileset::metatiles`). See
 * docs/portable_rom_map_layout_metatiles.md.
 */
const u16 *FireredMapLayoutMetatileGraphicsPtrForPrimary(const struct MapLayout *layout);
const u16 *FireredMapLayoutMetatileGraphicsPtrForSecondary(const struct MapLayout *layout);

const u16 *FireredMapLayoutMetatileMapPtrForLayoutId(u16 mapLayoutId);
const u16 *FireredMapLayoutMetatileBorderPtrForLayoutId(u16 mapLayoutId);

/*
 * Per-slot effective `MapLayout` (EWRAM): optional ROM dimensions, ROM metatile
 * pointers, and optional ROM tileset indices into `gFireredPortableCompiledTilesetTable[]`.
 * Before refresh, falls back to compiled rodata. See `docs/portable_rom_map_layout_metatiles.md`.
 */
const struct MapLayout *FireredPortableEffectiveMapLayoutForLayoutId(u16 mapLayoutId);

const u16 *FireredMapLayoutMetatileMapPtr(const struct MapLayout *layout);
const u16 *FireredMapLayoutMetatileBorderPtr(const struct MapLayout *layout);

/*
 * Resolve layout **map** / **border** grids: `u16` block words per `global.fieldmap.h`.
 * Metatile id semantics: `docs/architecture/project_c_metatile_id_map_format.md`;
 * changing word layout: `docs/architecture/project_c_map_block_word_and_tooling_boundary.md`.
 */
#define MAP_LAYOUT_METATILE_MAP_PTR(ml) FireredMapLayoutMetatileMapPtr(ml)
#define MAP_LAYOUT_METATILE_BORDER_PTR(ml) FireredMapLayoutMetatileBorderPtr(ml)
#else

#define FireredRomMapLayoutMetatilesRomActive() FALSE
#define FireredRomMapLayoutTilesetIndicesRomActive() FALSE
#define FireredRomCompiledTilesetAssetProfileRomActive() FALSE
#define FireredRomFieldMapPalettePolicyRomActive() FALSE
#define FireredPortableFieldMapBgPalettePrimaryRows() ((u8)NUM_PALS_IN_PRIMARY)
#define FireredPortableFieldMapBgPaletteSecondaryRows() ((u8)((NUM_PALS_TOTAL) - (NUM_PALS_IN_PRIMARY)))
#define FireredPortableCompiledTilesetVramTileBytesForLoad(ts, n) ((u32)(n) * 32u)

#define FireredPortableCompiledTilesetPaletteRowPtr(ts, row) \
    ((ts) != NULL ? (ts)->palettes[(row)] : NULL)

/* Compiled `MapLayout::map` / `::border`: `u16` block words (`global.fieldmap.h` / Project C docs). */
#define MAP_LAYOUT_METATILE_MAP_PTR(ml) ((ml)->map)
#define MAP_LAYOUT_METATILE_BORDER_PTR(ml) ((ml)->border)
#endif

#endif /* GUARD_MAP_LAYOUT_METATILES_ACCESS_H */
