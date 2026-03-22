#ifndef GUARD_HEAL_LOCATION_H
#define GUARD_HEAL_LOCATION_H

#include "global.h"

struct HealLocation
{
    s8 mapGroup;
    s8 mapNum;
    s16 x;
    s16 y;
};

#ifdef PORTABLE
extern const struct HealLocation *gHealLocationsActive;
/* Both NULL unless ROM loaded **both** whiteout companion blobs (same row count as heal). */
extern const u16 (*gWhiteoutRespawnHealCenterMapIdxsActive)[2];
extern const u8 *gWhiteoutRespawnHealerNpcIdsActive;
#endif

const struct HealLocation *GetHealLocation(u32 loc);
void SetWhiteoutRespawnWarpAndHealerNpc(struct WarpData * warp);

#endif // GUARD_HEAL_LOCATION_H
