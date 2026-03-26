#ifndef GUARD_MAP_CONNECTIONS_ACCESS_H
#define GUARD_MAP_CONNECTIONS_ACCESS_H

#include "global.h"

struct MapHeader;

#ifdef PORTABLE

void firered_portable_rom_map_connections_refresh_after_rom_load(void);

/* After copying *gMapGroups[g][n] into gMapHeader, apply ROM-backed connections when active. */
void firered_portable_rom_map_connections_merge(struct MapHeader *dst, u16 mapGroup, u16 mapNum);

#else

#define firered_portable_rom_map_connections_merge(dst, g, n) ((void)0)

#endif

#endif /* GUARD_MAP_CONNECTIONS_ACCESS_H */
