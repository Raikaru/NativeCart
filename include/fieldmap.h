#ifndef GUARD_FIELDMAP_H
#define GUARD_FIELDMAP_H

#include "global.h"
#include "map_grid_block_word.h"

#define NUM_TILES_IN_PRIMARY 640
#define NUM_TILES_TOTAL 1024
#define NUM_METATILES_IN_PRIMARY 640
/*
 * Compiled tileset metatile **table** row counts (640 primary + 384 secondary). Map **grid** ids may
 * use a wider encoded space (`MAP_GRID_METATILE_ID_SPACE_SIZE`); secondary rows beyond 384 require
 * extended ROM/profile tables (portable).
 */
#define NUM_METATILES_TOTAL 1024
#define NUM_PALS_IN_PRIMARY 7
#define NUM_PALS_TOTAL 13

/*
 * Project C — map grid **metatile ID** encoding (vanilla FireRed).
 *
 * Map grid cells store a `u16` **block word**; the **metatile id** is the low bits selected by
 * `MAPGRID_METATILE_ID_MASK` (see `fieldmap.c`). Vanilla interprets that id as a single contiguous
 * space split between two tilesets:
 *
 *   [0 .. NUM_METATILES_IN_PRIMARY-1]           → primary metatile **row** (graphics + attributes)
 *   [NUM_METATILES_IN_PRIMARY .. MAP_GRID_METATILE_ID_SPACE_SIZE - 1] → secondary metatile **row**
 *   (vanilla compiled secondary has **384** rows; ids **1024..2047** need extended tables).
 *
 * **Encoded space:** valid metatile ids are **`0 .. MAP_GRID_METATILE_ID_SPACE_SIZE - 1`** (same span as
 * **`MAPGRID_METATILE_ID_MASK`** in `global.fieldmap.h`). **`MapGridMetatileIdIsInEncodedSpace`** is the
 * single predicate for “in range for the vanilla map grid word”.
 *
 * **Out-of-encoded-space policy (Project C):** **`DrawMetatileAt`** and **shop / Teachy TV** previews
 * normalize invalid ids to **0** (draw primary row 0) before primary vs non-primary branching.
 * **`GetAttributeByMetatileIdAndMapLayout`** returns **`0xFF`** for all attribute extractions when the id
 * is not in encoded space (equivalent to the former neither-primary-nor-secondary branch). Map data from
 * `MapGridGetMetatileIdAt` is always in encoded space today; the policy guards ROM evolution and raw callers.
 *
 * This is **orthogonal** to Project B’s **tile index** / char-map policy (`MAP_BG_*` tile counts,
 * `FieldMapTranslateMetatileTileHalfwordForFieldBg`): metatile IDs pick **which metatile row**; each
 * metatile’s halfwords then reference **tile indices** into the combined field char block.
 *
 * ROM/profile tables may allocate **more** than vanilla rows for portable builds. Phase 2 map **grid**
 * encoding supports **2048** ids; on-disk `map.bin` stays PRET wire V1 — see
 * `docs/architecture/project_c_metatile_id_map_format.md`.
 */
#define MAP_GRID_METATILE_PRIMARY_COUNT    NUM_METATILES_IN_PRIMARY
#define MAP_GRID_METATILE_ID_SPACE_SIZE    2048
#define MAP_GRID_METATILE_SECONDARY_COUNT  (MAP_GRID_METATILE_ID_SPACE_SIZE - MAP_GRID_METATILE_PRIMARY_COUNT)

STATIC_ASSERT(MAP_GRID_METATILE_PRIMARY_COUNT + MAP_GRID_METATILE_SECONDARY_COUNT == MAP_GRID_METATILE_ID_SPACE_SIZE,
              map_grid_metatile_primary_plus_secondary_eq_space);

static inline bool32 MapGridMetatileIdUsesPrimaryTileset(u32 metatileId)
{
    return metatileId < MAP_GRID_METATILE_PRIMARY_COUNT;
}

static inline bool32 MapGridMetatileIdUsesSecondaryTileset(u32 metatileId)
{
    return metatileId >= MAP_GRID_METATILE_PRIMARY_COUNT
        && metatileId < MAP_GRID_METATILE_ID_SPACE_SIZE;
}

static inline bool32 MapGridMetatileIdIsInEncodedSpace(u32 metatileId)
{
    return metatileId < MAP_GRID_METATILE_ID_SPACE_SIZE;
}

/*
 * `metatileId - MAP_GRID_METATILE_PRIMARY_COUNT` for any id that is **not** primary.
 * Use only in branches reached after **`MapGridMetatileIdIsInEncodedSpace`** normalization in **field_camera**
 * and previews; otherwise the offset can exceed secondary table bounds.
 */
static inline u32 MapGridMetatileNonPrimaryRowOffset(u32 metatileId)
{
    return metatileId - MAP_GRID_METATILE_PRIMARY_COUNT;
}

/* Secondary **row index** into `secondaryTileset` tables; call only when `MapGridMetatileIdUsesSecondaryTileset`. */
static inline u32 MapGridMetatileSecondaryRowIndex(u32 metatileId)
{
    return MapGridMetatileNonPrimaryRowOffset(metatileId);
}

/*
 * Overworld map tileset palette policy (field BG, primary + secondary `Tileset`s on BG2).
 *
 * Primary rows load into BG palette slots [0 .. NUM_PALS_IN_PRIMARY - 1] (dest base `BG_PLTT_ID(0)`).
 * Secondary rows load into slots [NUM_PALS_IN_PRIMARY .. NUM_PALS_TOTAL - 1]
 * (dest base `BG_PLTT_ID(MAP_BG_SECONDARY_PALETTE_START_SLOT)`).
 *
 * PORTABLE ROM palette bundles use the same row-major layout; the secondary tileset's first row in VRAM
 * is bundle row `MAP_BG_SECONDARY_TILESET_PALETTE_ROW_INDEX` (== `NUM_PALS_IN_PRIMARY` today).
 *
 * Changing counts or slot placement requires coordinated updates (tint, quest log, other BG users)
 * and possibly engine palette layout — see `docs/architecture/map_visual_three_frontiers.md`.
 *
 * PORTABLE: optional ROM word `FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF` can override the
 * primary/secondary **row split** at runtime (`FireredPortableFieldMapBgPalettePrimaryRows()` /
 * `FireredPortableFieldMapBgPaletteSecondaryRows()` in `map_layout_metatiles_access.h`). The
 * `MAP_BG_*` macros remain the **vanilla** compile-time policy surface for non-PORTABLE builds
 * and documentation.
 */
#define MAP_BG_SECONDARY_PALETTE_START_SLOT     (NUM_PALS_IN_PRIMARY)
#define MAP_BG_PRIMARY_PALETTE_BYTES            ((unsigned)(NUM_PALS_IN_PRIMARY) * PLTT_SIZE_4BPP)
#define MAP_BG_SECONDARY_PALETTE_BYTES          (((unsigned)(NUM_PALS_TOTAL) - (unsigned)(NUM_PALS_IN_PRIMARY)) * PLTT_SIZE_4BPP)
#define MAP_BG_SECONDARY_TILESET_PALETTE_ROW_INDEX (NUM_PALS_IN_PRIMARY)

/*
 * Overworld map tileset **char** layout (combined 4bpp tile indices on the field BG path, vanilla BG2).
 *
 * Primary tileset occupies `MAP_BG_PRIMARY_TILESET_TILE_COUNT` tiles starting at
 * `MAP_BG_PRIMARY_TILESET_CHAR_BASE_TILE`. Secondary occupies `MAP_BG_SECONDARY_TILESET_TILE_COUNT`
 * tiles starting at `MAP_BG_SECONDARY_TILESET_CHAR_BASE_TILE` (contiguous after primary).
 * `MAP_BG_FIELD_TILE_CHAR_CAPACITY` is the vanilla combined tile index space (metatile graphics
 * still assume this ceiling via `NUM_METATILES_*` / Project C).
 *
 * `Copy*TilesetToVram*` in `fieldmap.c`, PORTABLE `FireredPortableCompiledTilesetVramTileBytesForLoad`,
 * door animation scratch tiles, and Teachy TV staging should use these names so future **VRAM /
 * BG char policy** (Project B) has one edit surface — see `docs/architecture/map_visual_three_frontiers.md`.
 * PORTABLE: `src_transformed/fieldmap.c` **asserts** (debug) or **skips upload** (NDEBUG) when profile
 * sizes would break **`LoadBgTiles` `u16`** limits or **`MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES`** /
 * **`MAP_BG_FIELD_TILE_CHAR_CAPACITY`**; `dma3_manager.c` rejects DMA past emulated VRAM/PLTT/OAM.
 */
#define MAP_BG_PRIMARY_TILESET_CHAR_BASE_TILE       0u
#define MAP_BG_SECONDARY_TILESET_CHAR_BASE_TILE     (NUM_TILES_IN_PRIMARY)
#define MAP_BG_PRIMARY_TILESET_TILE_COUNT           (NUM_TILES_IN_PRIMARY)
#define MAP_BG_SECONDARY_TILESET_TILE_COUNT         ((NUM_TILES_TOTAL) - (NUM_TILES_IN_PRIMARY))
#define MAP_BG_FIELD_TILE_CHAR_CAPACITY             (NUM_TILES_TOTAL)
/*
 * Byte span of the combined field map tileset in char VRAM (4bpp), from the field BG’s char base.
 * Must stay within emulated GBA VRAM (`0x18000` bytes total — see portable DMA guard in `dma3_manager.c`).
 */
#define MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES        ((u32)(MAP_BG_FIELD_TILE_CHAR_CAPACITY) * (u32)TILE_SIZE_4BPP)
/* Which GBA BG (`LoadBgTiles` / `Decompress*BgGfx*2` first argument) holds the combined map tileset chars. */
#define MAP_BG_FIELD_TILESET_GFX_BG                 2u

STATIC_ASSERT(MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES <= 0x18000u, map_field_tileset_char_vram_bytes_fit_gba_vram);
/*
 * `BgTemplate.charBaseIndex` for the field map tileset BG (`MAP_BG_FIELD_TILESET_GFX_BG`).
 * Must match `sOverworldBgTemplates[MAP_BG_FIELD_TILESET_GFX_BG]` in `overworld.c` — PORTABLE
 * `InitOverworldBgs` / `InitOverworldBgs_NoResetHeap` assert agreement after `InitBgsFromTemplates`.
 */
#define MAP_BG_FIELD_TILESET_CHAR_BASE_BLOCK        0u

/*
 * **Char-block vs linear tile index** (GBA text-mode BG, 4bpp):
 *
 * `MAP_BG_FIELD_TILESET_CHAR_BASE_BLOCK` picks which 16KiB char **origin** in VRAM (`BG_CHAR_SIZE`)
 * tilemap tile numbers are measured from. The hardware resolves tile index *T* to byte offset
 * `charBaseBlock * BG_CHAR_SIZE + T * TILE_SIZE_4BPP` from the start of BG tile VRAM (same formula as
 * portable `engine_render_4bpp_tile`: `(BGCNT char base) * 0x4000 + tile_index * 32`).
 *
 * FireRed’s field path uses **one** contiguous index range [0 .. `MAP_BG_FIELD_TILESET_MAX_TILE_INDEX`]
 * for the combined primary+secondary map chars. That **linear** byte span (`MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES`)
 * may cross the boundary between adjacent 16KiB slabs; vanilla spans **two** `BG_CHAR_SIZE` units from base 0.
 *
 * Standard BG tilemap entries use a **10-bit** tile number (0..1023). Capacity above 1024, multiple disjoint
 * char bases for one logical tileset, or different metatile→tile encoding are **not** covered here — they need
 * coordinated tilemap, `field_camera`, upload, and renderer changes (natural Project B / engine frontier).
 */
#define MAP_BG_FIELD_TILESET_MAX_TILE_INDEX     ((MAP_BG_FIELD_TILE_CHAR_CAPACITY) - 1u)

/*
 * Door animation **scratch** tiles (`field_door.c`): 8 char slots at the **end** of the combined field
 * tile index range. **`CopyDoorTilesToVram`** uploads to **`DOOR_TILE_START`** (= this first index);
 * **`BuildDoorTiles`** emits halfwords **`DOOR_TILE_START + i`** which pass through
 * **`FieldMapTranslateMetatileTileHalfwordForFieldBg`**. Until door upload + halfword construction share
 * the same remapping policy as metatile gfx, **each reserved logical index must map to itself**
 * (`gFieldMapLogicalToPhysicalTileId[i] == i` for `i` in this range). See Direction B docs.
 */
#define MAP_BG_FIELD_DOOR_TILE_RESERVED_COUNT  8u
#define MAP_BG_FIELD_DOOR_TILE_RESERVED_FIRST  ((u16)((MAP_BG_FIELD_TILE_CHAR_CAPACITY) - (MAP_BG_FIELD_DOOR_TILE_RESERVED_COUNT)))

STATIC_ASSERT((u32)(MAP_BG_FIELD_DOOR_TILE_RESERVED_FIRST) + (MAP_BG_FIELD_DOOR_TILE_RESERVED_COUNT) == (u32)(MAP_BG_FIELD_TILE_CHAR_CAPACITY),
              map_bg_field_door_reserved_range_covers_tile_capacity_tail);

STATIC_ASSERT(MAP_BG_FIELD_TILE_CHAR_CAPACITY <= 1024u, map_field_tile_capacity_within_bg_tilemap_10bit_space);
STATIC_ASSERT(MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES <= 2u * (u32)BG_CHAR_SIZE,
              map_field_tileset_span_at_most_two_hw_char_blocks_from_base);

/*
 * Direction B — documented logical→physical **TID** contract for the field map BG.
 *
 * Runtime uses **`gFieldMapLogicalToPhysicalTileId`** plus **`FieldMapPhysicalTileIndexFromLogical`** /
 * **`FieldMapTranslateMetatileTileHalfwordForFieldBg`** (tilemap buffers hold **physical** TIDs after draw).
 * This aggregate names the mapping data for docs / future ROM-pack versioning (not yet wired as a runtime handle).
 */
struct FireredFieldBgTileAddressing
{
    const u16 *logicalToPhysicalTileId10; /* length `logicalTileCapacity` */
    u16 logicalTileCapacity; /* `MAP_BG_FIELD_TILE_CHAR_CAPACITY` for the shipped table */
};

/*
 * Direction B — logical → physical tile **TID** for the field map BG path (identity today).
 *
 * Metatile graphics tables emit standard GBA **tilemap halfwords**: bits 0–9 = tile number (TID),
 * 10–11 = flip, 12–15 = palette. **`FieldMapTranslateMetatileTileHalfwordForFieldBg`** is the
 * choke point where a future non-identity map (banking, remapping) can substitute **physical** TIDs
 * before VRAM tilemaps are written. **`Copy*TilesetToVram*` / `CopyMapTilesetsToVram`** lay out **4bpp**
 * tile **chars** in field BG VRAM so **physical** slot **`gFieldMapLogicalToPhysicalTileId[logical]`**
 * holds the pixels for **combined** logical index **`logical`** (primary **0..639**, secondary **640..1023**).
 * Identity layout keeps the historical **one bulk `LoadBgTiles`** / decompress path; non-identity uses a
 * bounded per-tile upload (compressed remapped path **synchronously** waits on each DMA before freeing
 * the decompressed buffer).
 *
 * **`gFieldMapLogicalToPhysicalTileId`** is the first mapping-data hook: vanilla ships an **identity**
 * table (`[i] == i`); hacks/tools may replace this TU or regenerate the initializer while keeping the
 * writer surface (`FieldMapTranslateMetatileTileHalfwordForFieldBg`) unchanged.
 *
 * **Door scratch:** **`MAP_BG_FIELD_DOOR_TILE_RESERVED_*`** — until door VRAM upload uses the same
 * remapping as this table, those logical indices must stay **identity** (see `FieldMapVerifyLogicalToPhysicalTileTable`).
 *
 * **Table readiness (offline):** `tools/validate_field_map_logical_to_physical_inc.py` or **`tools/check_direction_b_offline.py`** — door tail, 10-bit range,
 * optional **`--strict-identity`**, collision **warnings** (or **`--strict-injective`** to error on shared physical TIDs),
 * optional **`--hint-primary-secondary`** for cross-primary/secondary physical placement notes.
 */
extern const u16 gFieldMapLogicalToPhysicalTileId[MAP_BG_FIELD_TILE_CHAR_CAPACITY];

#if defined(PORTABLE) && !defined(NDEBUG)
void FieldMapVerifyLogicalToPhysicalTileTable(void);
#else
static inline void FieldMapVerifyLogicalToPhysicalTileTable(void) { }
#endif

bool32 FieldMapFieldTileUploadUsesIdentityLayout(void);

static inline u16 FieldMapPhysicalTileIndexFromLogical(u16 logicalTileTid10)
{
    u16 log = (u16)(logicalTileTid10 & 0x3FFu);

    return (u16)(gFieldMapLogicalToPhysicalTileId[log] & 0x3FFu);
}

static inline u16 FieldMapTranslateMetatileTileHalfwordForFieldBg(u16 metatileGfxHalfword)
{
    u16 tid = metatileGfxHalfword & 0x3FFu;
    u16 attrs = (u16)(metatileGfxHalfword & (u16)~0x3FFu);
    u16 phys = (u16)(FieldMapPhysicalTileIndexFromLogical(tid) & 0x3FFu);

    /* Direction D: physical TID must fit standard map BG tilemap + MAP_BG char layout (debug). */
#if !defined(NDEBUG)
    AGB_ASSERT(phys <= MAP_BG_FIELD_TILESET_MAX_TILE_INDEX);
#endif
    return (u16)(attrs | phys);
}

#define MAX_MAP_DATA_SIZE 0x2800
#define VIRTUAL_MAP_SIZE (MAX_MAP_DATA_SIZE)

#define NUM_TILES_PER_METATILE 8

// Map coordinates are offset by 7 when using the map
// buffer because it needs to load sufficient border
// metatiles to fill the player's view (the player has
// 7 metatiles of view horizontally in either direction).
#define MAP_OFFSET 7
#define MAP_OFFSET_W (MAP_OFFSET * 2 + 1)
#define MAP_OFFSET_H (MAP_OFFSET * 2)

extern struct BackupMapLayout VMap;
extern const struct MapLayout Route1_Layout;

/* Map grid cells are `u16` block words (`global.fieldmap.h`); metatile policy: `MAP_GRID_METATILE_*` below. */
u32 MapGridGetMetatileIdAt(s32, s32);
u32 MapGridGetMetatileBehaviorAt(s16, s16);
u8 MapGridGetMetatileLayerTypeAt(s16 x, s16 y);
void MapGridSetMetatileIdAt(s32, s32, u16);
void MapGridSetMetatileEntryAt(s32, s32, u16);
u8 MapGridGetElevationAt(s32 x, s32 y);
void GetCameraCoords(u16 *, u16 *);
bool8 MapGridGetCollisionAt(s32, s32);
s32 GetMapBorderIdAt(s32, s32);
bool32 CanCameraMoveInDirection(s32);
const struct MapHeader * GetMapHeaderFromConnection(const struct MapConnection * connection);
const struct MapConnection * GetMapConnectionAtPos(s16 x, s16 y);
void ApplyGlobalTintToPaletteSlot(u8 slot, u8 count);
void SaveMapView(void);
u32 ExtractMetatileAttribute(u32 attributes, u8 attributeType);
u32 MapGridGetMetatileAttributeAt(s16 x, s16 y, u8 attributeType);
void MapGridSetMetatileImpassabilityAt(s32 x, s32 y, bool32 arg2);
bool8 CameraMove(s32 x, s32 y);
void CopyMapTilesetsToVram(struct MapLayout const * mapLayout);
void LoadMapTilesetPalettes(struct MapLayout const * mapLayout);
void InitMap(void);
void CopySecondaryTilesetToVramUsingHeap(const struct MapLayout * mapLayout);
void LoadSecondaryTilesetPalette(const struct MapLayout * mapLayout);
void InitMapFromSavedGame(void);
void CopyPrimaryTilesetToVram(const struct MapLayout *mapLayout);
void CopySecondaryTilesetToVram(const struct MapLayout *mapLayout);
void GetCameraFocusCoords(u16 *x, u16 *y);
void SetCameraFocusCoords(u16 x, u16 y);

#endif //GUARD_FIELDMAP_H
