#ifndef GUARD_MAP_HEADER_SCALARS_ACCESS_H
#define GUARD_MAP_HEADER_SCALARS_ACCESS_H

#include "global.h"

struct MapHeader;

#ifdef PORTABLE

void firered_portable_rom_map_header_scalars_refresh_after_rom_load(void);

void firered_portable_rom_map_header_scalars_merge(struct MapHeader *dst, u16 mapGroup, u16 mapNum);

u16 FireredRomMapHeaderScalarsMusic(u16 mapGroup, u16 mapNum, u16 fallback);
u8 FireredRomMapHeaderScalarsRegionMapSec(u16 mapGroup, u16 mapNum, u8 fallback);
u8 FireredRomMapHeaderScalarsMapType(u16 mapGroup, u16 mapNum, u8 fallback);
u8 FireredRomMapHeaderScalarsBattleType(u16 mapGroup, u16 mapNum, u8 fallback);

#else

#define firered_portable_rom_map_header_scalars_merge(dst, g, n) ((void)0)
#define FireredRomMapHeaderScalarsMusic(g, n, fb) (fb)
#define FireredRomMapHeaderScalarsRegionMapSec(g, n, fb) (fb)
#define FireredRomMapHeaderScalarsMapType(g, n, fb) (fb)
#define FireredRomMapHeaderScalarsBattleType(g, n, fb) (fb)

#endif

#endif /* GUARD_MAP_HEADER_SCALARS_ACCESS_H */
