#ifndef GUARD_REGION_MAP_MAPSEC_NAMES_ROM_H
#define GUARD_REGION_MAP_MAPSEC_NAMES_ROM_H

#include "global.h"

#include "constants/region_map_mapsec_names.h"

#ifdef PORTABLE
/*
 * After ROM load refresh: each entry is either a scanned/env/profile match in the mapped ROM
 * or the compiled `gRegionMapMapsecNames_Compiled[i]` pointer (GBA-encoded, EOS-terminated).
 */
extern const u8 *gRegionMapMapsecNamesResolved[REGION_MAP_MAPSEC_NAME_ENTRY_COUNT];
#endif

#endif /* GUARD_REGION_MAP_MAPSEC_NAMES_ROM_H */
