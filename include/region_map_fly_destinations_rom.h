#ifndef GUARD_REGION_MAP_FLY_DESTINATIONS_ROM_H
#define GUARD_REGION_MAP_FLY_DESTINATIONS_ROM_H

#include "global.h"

#include "constants/region_map_fly_destinations.h"

#ifdef PORTABLE
/* NULL when using compiled `sMapFlyDestinations_Compiled`. */
extern const u8 (*gMapFlyDestinationsActive)[3];
#endif

#endif /* GUARD_REGION_MAP_FLY_DESTINATIONS_ROM_H */
