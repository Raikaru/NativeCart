#ifndef GUARD_CONSTANTS_REGION_MAP_MAPSEC_NAMES_H
#define GUARD_CONSTANTS_REGION_MAP_MAPSEC_NAMES_H

#include "constants/region_map_sections.h"

/*
 * Row count for `gRegionMapMapsecNames_Compiled[]` / `gRegionMapMapsecNamesResolved[]`.
 * Matches `GetMapName` index bound: `mapsec - KANTO_MAPSEC_START < MAPSEC_NONE - KANTO_MAPSEC_START`.
 */
#define REGION_MAP_MAPSEC_NAME_ENTRY_COUNT ((unsigned)(MAPSEC_NONE - KANTO_MAPSEC_START))

#endif /* GUARD_CONSTANTS_REGION_MAP_MAPSEC_NAMES_H */
