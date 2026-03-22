#ifndef GUARD_REGION_MAP_SECTION_GRIDS_ROM_H
#define GUARD_REGION_MAP_SECTION_GRIDS_ROM_H

#include "global.h"

#include "constants/region_map_layout_dims.h"

#ifdef PORTABLE
/*
 * All four are non-NULL after a successful ROM pack load, else all NULL (compiled `*_Compiled` tensors).
 * See `docs/portable_rom_region_map_section_grids.md`.
 */
extern const u8 (*gRegionMapSectionGridKantoActive)[REGION_MAP_SECTION_GRID_HEIGHT][REGION_MAP_SECTION_GRID_WIDTH];
extern const u8 (*gRegionMapSectionGridSevii123Active)[REGION_MAP_SECTION_GRID_HEIGHT][REGION_MAP_SECTION_GRID_WIDTH];
extern const u8 (*gRegionMapSectionGridSevii45Active)[REGION_MAP_SECTION_GRID_HEIGHT][REGION_MAP_SECTION_GRID_WIDTH];
extern const u8 (*gRegionMapSectionGridSevii67Active)[REGION_MAP_SECTION_GRID_HEIGHT][REGION_MAP_SECTION_GRID_WIDTH];
#endif

#endif /* GUARD_REGION_MAP_SECTION_GRIDS_ROM_H */
