#ifndef GUARD_CONSTANTS_REGION_MAP_FLY_DESTINATIONS_H
#define GUARD_CONSTANTS_REGION_MAP_FLY_DESTINATIONS_H

#include "constants/region_map_sections.h"

/*
 * Rows of `sMapFlyDestinations[mapsec - KANTO_MAPSEC_START][3]` in region_map.c
 * (group, num, healLocation). Last populated mapsec in vanilla table is EMBER_SPA.
 */
#define REGION_MAP_FLY_DESTINATION_ROW_COUNT (MAPSEC_EMBER_SPA - KANTO_MAPSEC_START + 1)

#endif /* GUARD_CONSTANTS_REGION_MAP_FLY_DESTINATIONS_H */
