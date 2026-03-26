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

/*
 * When the scalar pack is active, returns the wire `mapLayoutId` for (group,num);
 * otherwise `compiledLayoutId`. Used with layout-metatile ROM so connection fill
 * and metatile grids agree with ROM-backed rewires without merging rodata headers.
 */
u16 FireredRomMapHeaderScalarsEffectiveMapLayoutId(u16 mapGroup, u16 mapNum, u16 compiledLayoutId);

#else

#define firered_portable_rom_map_header_scalars_merge(dst, g, n) ((void)0)
#define FireredRomMapHeaderScalarsMusic(g, n, fb) (fb)
#define FireredRomMapHeaderScalarsRegionMapSec(g, n, fb) (fb)
#define FireredRomMapHeaderScalarsMapType(g, n, fb) (fb)
#define FireredRomMapHeaderScalarsBattleType(g, n, fb) (fb)
#define FireredRomMapHeaderScalarsEffectiveMapLayoutId(g, n, compiled) (compiled)

#endif

#endif /* GUARD_MAP_HEADER_SCALARS_ACCESS_H */
