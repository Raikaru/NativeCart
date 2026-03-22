#ifndef GUARD_REGION_MAP_SECTION_LAYOUT_ROM_H
#define GUARD_REGION_MAP_SECTION_LAYOUT_ROM_H

#include "global.h"

#include "constants/region_map_sections.h"

#ifdef PORTABLE
/*
 * Both NULL unless ROM loaded the **layout pack** (corners + dimensions, `MAPSEC_COUNT` rows each).
 * See `docs/portable_rom_region_map_section_layout.md`.
 */
extern const u16 (*gMapSectionTopLeftCornersActive)[2];
extern const u16 (*gMapSectionDimensionsActive)[2];
#endif

#endif /* GUARD_REGION_MAP_SECTION_LAYOUT_ROM_H */
