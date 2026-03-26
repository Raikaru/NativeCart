#include "global.h"
#include "fieldmap.h"

#if MAP_BG_FIELD_TILE_CHAR_CAPACITY != 1024
#error "Regenerate src/data/field_map_logical_to_physical_identity.inc when MAP_BG_FIELD_TILE_CHAR_CAPACITY changes."
#endif

/*
 * Direction B — logical (metatile graphics) → physical BG tilemap TID.
 * Identity today; replace or regenerate this table for non-identity layouts once upload/VRAM policy match.
 * Regenerate initializer: `python tools/generate_field_map_logical_to_physical_identity_inc.py`.
 * Table checks: `python tools/validate_field_map_logical_to_physical_inc.py` (see `FieldMapVerifyLogicalToPhysicalTileTable` in **PORTABLE** non-**NDEBUG** builds;
 * optional `--strict-injective`, collision warnings, `--hint-primary-secondary` — see `include/fieldmap.h` / Project B docs).
 */
const u16 gFieldMapLogicalToPhysicalTileId[MAP_BG_FIELD_TILE_CHAR_CAPACITY] = {
#include "data/field_map_logical_to_physical_identity.inc"
};

bool32 FieldMapFieldTileUploadUsesIdentityLayout(void)
{
    static u8 sInited;
    static u8 sIsIdentity;
    u16 i;

    if (sInited)
        return sIsIdentity;
    sInited = 1;
    for (i = 0; i < MAP_BG_FIELD_TILE_CHAR_CAPACITY; i++)
    {
        if ((gFieldMapLogicalToPhysicalTileId[i] & 0x3FFu) != i)
        {
            sIsIdentity = FALSE;
            return FALSE;
        }
    }
    sIsIdentity = TRUE;
    return TRUE;
}

#if defined(PORTABLE) && !defined(NDEBUG)
void FieldMapVerifyLogicalToPhysicalTileTable(void)
{
    static u8 sVerifiedOnce;
    u16 i;
    u8 sawNonIdentity;

    if (sVerifiedOnce)
        return;
    sVerifiedOnce = 1;

    sawNonIdentity = 0;
    for (i = 0; i < MAP_BG_FIELD_TILE_CHAR_CAPACITY; i++)
    {
        u16 raw = gFieldMapLogicalToPhysicalTileId[i];
        u16 phys = (u16)(raw & 0x3FFu);

        AGB_ASSERT(raw == phys);
        AGB_ASSERT(phys <= MAP_BG_FIELD_TILESET_MAX_TILE_INDEX);
        if (phys != i)
            sawNonIdentity = 1;
    }

    for (i = MAP_BG_FIELD_DOOR_TILE_RESERVED_FIRST; i < MAP_BG_FIELD_TILE_CHAR_CAPACITY; i++)
        AGB_ASSERT(gFieldMapLogicalToPhysicalTileId[i] == i);

    /*
     * Non-identity tables: one-shot warning if two logicals share a physical TID (upload last-wins).
     * Offline: `tools/validate_field_map_logical_to_physical_inc.py` (--strict-injective for CI hard fail).
     */
    if (sawNonIdentity)
    {
        u8 used[(MAP_BG_FIELD_TILE_CHAR_CAPACITY + 7) / 8];
        u16 p;
        u16 byte;
        u8 mask;

        memset(used, 0, sizeof(used));
        for (i = 0; i < MAP_BG_FIELD_TILE_CHAR_CAPACITY; i++)
        {
            p = (u16)(gFieldMapLogicalToPhysicalTileId[i] & 0x3FFu);
            byte = (u16)(p >> 3);
            mask = (u8)(1u << (p & 7));
            if ((used[byte] & mask) != 0)
            {
                AGB_WARNING(FALSE);
                break;
            }
            used[byte] |= mask;
        }
    }
}
#endif /* PORTABLE && !NDEBUG */
