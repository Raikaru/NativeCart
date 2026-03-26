#ifndef GUARD_MAP_SCRIPTS_ACCESS_H
#define GUARD_MAP_SCRIPTS_ACCESS_H

#include "global.h"

struct MapHeader;

#ifdef PORTABLE

void firered_portable_rom_map_scripts_directory_refresh_after_rom_load(void);

/* After copying *gMapGroups[g][n] into gMapHeader; optional ROM override for mapScripts pointer. */
void firered_portable_rom_map_scripts_directory_merge(struct MapHeader *dst, u16 mapGroup, u16 mapNum);

#else

#define firered_portable_rom_map_scripts_directory_merge(dst, g, n) ((void)0)

#endif

#endif /* GUARD_MAP_SCRIPTS_ACCESS_H */
