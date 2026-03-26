#ifndef GUARD_MAP_GRID_BLOCK_WORD_H
#define GUARD_MAP_GRID_BLOCK_WORD_H

/*
 * Canonical pack/unpack for map grid **`u16` block words** (Project C Phase 2 RAM layout).
 * Masks: `global.fieldmap.h` / `constants/map_grid_block_word.h`. On-disk `map.bin` / `border.bin`
 * stay **PRET wire V1**; convert with **`MapGridBlockWordFromWireV1`** before mixing into `VMap`.
 * Save `mapView` uses Phase 2 after **`NormalizeSavedMapViewBlockWords`** (`mapGridBlockWordSaveLayout`).
 *
 * **Project B:** metatile **tilemap halfwords** (10-bit TID in gfx tables) are separate from these
 * block-word metatile ids — see `fieldmap.h` / `FieldMapTranslateMetatileTileHalfwordForFieldBg`.
 */

#include "constants/map_grid_block_word.h"

#ifndef MAPGRID_METATILE_ID_MASK
#error "Include global.h (or global.fieldmap.h after GBA types) before map_grid_block_word.h"
#endif

/* PRET `map.bin` / `border.bin` / legacy `mapView` (layout id V1) -> Phase 2 RAM word. */
static inline u16 MapGridBlockWordFromWireV1(u16 w)
{
    u32 mt = (u32)(w & MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK);
    u32 col = ((u32)w >> MAP_GRID_BLOCK_WORD_WIRE_V1_COLLISION_SHIFT) & 3u;
    u32 el = ((u32)w >> MAP_GRID_BLOCK_WORD_WIRE_V1_ELEVATION_SHIFT) & 0xFu;

    if (el > 7u)
        el = 7u;
    return (u16)(mt | (col << MAP_GRID_BLOCK_WORD_COLLISION_SHIFT) | (el << MAP_GRID_BLOCK_WORD_ELEVATION_SHIFT));
}

static inline bool32 MapGridBlockWordIsUndefined(u16 word)
{
    return word == MAPGRID_UNDEFINED;
}

/* Low metatile+collision field (bits not covered by `MAPGRID_ELEVATION_MASK`). */
static inline u16 MapGridBlockWordMetatileAndCollisionField(u16 word)
{
    return (u16)(word & (u16)~MAPGRID_ELEVATION_MASK);
}

static inline u32 MapGridBlockWordGetMetatileIdFromWord(u16 word)
{
    return (u32)(word & MAPGRID_METATILE_ID_MASK);
}

/*
 * Elevation nibble; `MAPGRID_UNDEFINED` → 0 (matches `MapGridGetElevationAt`).
 */
static inline u8 MapGridBlockWordGetElevation(u16 word)
{
    if (MapGridBlockWordIsUndefined(word))
        return 0;

    return (u8)(word >> MAPGRID_ELEVATION_SHIFT);
}

/*
 * Collision field 0..3; `MAPGRID_UNDEFINED` → treated as impassable (matches `MapGridGetCollisionAt`).
 */
static inline u8 MapGridBlockWordGetCollision(u16 word)
{
    if (MapGridBlockWordIsUndefined(word))
        return TRUE;

    return (u8)((word & MAPGRID_COLLISION_MASK) >> MAPGRID_COLLISION_SHIFT);
}

/*
 * Replace metatile id + collision bits; keep elevation bits from `block` (matches `MapGridSetMetatileIdAt`).
 * `metatileAndCollision` is typically `metatileId | MAPGRID_COLLISION_MASK` etc.
 */
static inline u16 MapGridBlockWordReplaceMetatileAndCollisionKeepingElevation(u16 block, u16 metatileAndCollision)
{
    return (u16)((block & MAPGRID_ELEVATION_MASK) | (metatileAndCollision & (u16)~MAPGRID_ELEVATION_MASK));
}

static inline u16 MapGridBlockWordSetImpassable(u16 block, bool32 impassable)
{
    if (impassable)
        return (u16)(block | MAPGRID_COLLISION_MASK);
    else
        return (u16)(block & (u16)~MAPGRID_COLLISION_MASK);
}

/*
 * Out-of-bounds / border-wrap path (`GetBorderBlockAt`): vanilla **forces** both collision bits on
 * the metatile read from `border.bin` / ROM border blob so wrap tiles behave impassable regardless
 * of stored collision nibble.
 */
static inline u16 MapGridBlockWordApplyBorderWrapCollision(u16 storedBorderBlockWord)
{
    return (u16)(storedBorderBlockWord | MAPGRID_COLLISION_MASK);
}

#endif // GUARD_MAP_GRID_BLOCK_WORD_H
