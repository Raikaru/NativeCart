#ifndef GUARD_MAP_LAYOUT_METATILES_ACCESS_H
#define GUARD_MAP_LAYOUT_METATILES_ACCESS_H

#include "global.h"

struct MapLayout;

/*
 * PORTABLE: optional ROM-backed u16 border + map metatile blobs (see
 * docs/portable_rom_map_layout_metatiles.md). Accessors resolve layout pointer →
 * slot index via a sorted table built at ROM load (O(log N) per call when active).
 */
#ifdef PORTABLE
void firered_portable_rom_map_layout_metatiles_refresh_after_rom_load(void);

const u16 *FireredMapLayoutMetatileMapPtr(const struct MapLayout *layout);
const u16 *FireredMapLayoutMetatileBorderPtr(const struct MapLayout *layout);

#define MAP_LAYOUT_METATILE_MAP_PTR(ml) FireredMapLayoutMetatileMapPtr(ml)
#define MAP_LAYOUT_METATILE_BORDER_PTR(ml) FireredMapLayoutMetatileBorderPtr(ml)
#else

#define MAP_LAYOUT_METATILE_MAP_PTR(ml) ((ml)->map)
#define MAP_LAYOUT_METATILE_BORDER_PTR(ml) ((ml)->border)
#endif

#endif /* GUARD_MAP_LAYOUT_METATILES_ACCESS_H */
