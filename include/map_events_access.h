#ifndef GUARD_MAP_EVENTS_ACCESS_H
#define GUARD_MAP_EVENTS_ACCESS_H

#include "global.h"

struct MapHeader;

#ifdef PORTABLE

void firered_portable_rom_map_events_directory_refresh_after_rom_load(void);

/*
 * After copying *gMapGroups[g][n] into gMapHeader (and, in src_transformed/overworld.c,
 * after ResolveRomPointer(gMapHeader.events)), optional ROM override for the full
 * MapEvents aggregate (objects / warps / coord / bg).
 */
void firered_portable_rom_map_events_directory_merge(struct MapHeader *dst, u16 mapGroup, u16 mapNum);

#else

#define firered_portable_rom_map_events_directory_merge(dst, g, n) ((void)0)

#endif

#endif /* GUARD_MAP_EVENTS_ACCESS_H */
