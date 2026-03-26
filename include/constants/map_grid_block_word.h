#ifndef GUARD_CONSTANTS_MAP_GRID_BLOCK_WORD_H
#define GUARD_CONSTANTS_MAP_GRID_BLOCK_WORD_H

/*
 * Map **`u16` block word** layouts (Project C).
 *
 * **On-disk / PRET wire (`data/layouts/<layout>/map.bin`, `border.bin`):** layout id **1** — 10-bit metatile,
 * 2-bit collision, 4-bit elevation. Runtime converts to **Phase 2 RAM** when copying into `VMap` /
 * `gBackupMapData` (`MapGridBlockWordFromWireV1` in `map_grid_block_word.h`).
 *
 * **In-RAM + `gSaveBlock2Ptr->mapView` (after migration):** layout id **2** — 11-bit metatile
 * (**0..2047**), 2-bit collision, **3-bit** elevation (**0..7**; vanilla FRLG map data uses **<= 7**).
 * `gSaveBlock1Ptr->mapGridBlockWordSaveLayout` records how `mapView` words are packed; see
 * `NormalizeSavedMapViewBlockWords` in `fieldmap.c`.
 *
 * See `docs/architecture/project_c_map_block_word_and_tooling_boundary.md`.
 */

/* PRET / retail on-disk and incbin layout (unchanged). */
#define MAP_GRID_BLOCK_WORD_LAYOUT_VANILLA 1

/* Runtime layout: wider metatile id field in the same `u16` cell. */
#define MAP_GRID_BLOCK_WORD_LAYOUT_PHASE2_U16 2

/* --- PRET wire V1 (on-disk) — used only by conversion helpers + tooling --- */
#define MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK   0x03FFu
#define MAP_GRID_BLOCK_WORD_WIRE_V1_COLLISION_MASK     0x0C00u
#define MAP_GRID_BLOCK_WORD_WIRE_V1_ELEVATION_MASK     0xF000u
#define MAP_GRID_BLOCK_WORD_WIRE_V1_COLLISION_SHIFT    10
#define MAP_GRID_BLOCK_WORD_WIRE_V1_ELEVATION_SHIFT    12

/*
 * Active RAM masks (`MAPGRID_*` in `global.fieldmap.h` aliases these).
 * `MAP_GRID_BLOCK_WORD_LAYOUT_PHASE2_U16`.
 */
#define MAP_GRID_BLOCK_WORD_METATILE_ID_MASK   0x07FFu
#define MAP_GRID_BLOCK_WORD_COLLISION_MASK     0x1800u
#define MAP_GRID_BLOCK_WORD_ELEVATION_MASK     0xE000u
#define MAP_GRID_BLOCK_WORD_COLLISION_SHIFT    11
#define MAP_GRID_BLOCK_WORD_ELEVATION_SHIFT    13
#define MAP_GRID_BLOCK_WORD_ENCODED_METATILE_COUNT 2048u

/*
 * Phase 3 — planned **on-disk `u32` cell** (not used by vanilla `mapjson` / runtime yet).
 * Little-endian word: 16-bit metatile id, 2-bit collision, 3-bit elevation, upper bits reserved.
 * Validate with `tools/validate_map_layout_block_bin.py --cell-width 4` when emitting `u32` layout bins.
 */
#define MAP_GRID_BLOCK_WORD_LAYOUT_PHASE3_U32 3
#define MAP_GRID_BLOCK_WORD_PHASE3_U32_METATILE_ID_MASK   0x0000FFFFu
#define MAP_GRID_BLOCK_WORD_PHASE3_U32_COLLISION_MASK       0x00030000u
#define MAP_GRID_BLOCK_WORD_PHASE3_U32_ELEVATION_MASK       0x001C0000u
#define MAP_GRID_BLOCK_WORD_PHASE3_U32_COLLISION_SHIFT    16
#define MAP_GRID_BLOCK_WORD_PHASE3_U32_ELEVATION_SHIFT    18
#define MAP_GRID_BLOCK_WORD_PHASE3_U32_ENCODED_METATILE_COUNT 65536u

#endif // GUARD_CONSTANTS_MAP_GRID_BLOCK_WORD_H
