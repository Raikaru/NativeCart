#include "global.h"

#include "map_header_scalars_access.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_map_header_scalars_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/map_groups.h"
#include "constants/region_map_sections.h"
#include "engine_internal.h"

typedef struct FireredRomMapHeaderScalarsWire {
    u16 music;
    u16 mapLayoutId;
    u8 regionMapSectionId;
    u8 cave;
    u8 weather;
    u8 mapType;
    u8 bikingAllowed;
    u8 allowEscaping;
    u8 allowRunning;
    u8 showMapName;
    s8 floorNum;
    u8 battleType;
    u8 _pad[2];
} FireredRomMapHeaderScalarsWire;

_Static_assert(sizeof(FireredRomMapHeaderScalarsWire) == 16u, "FireredRomMapHeaderScalarsWire must be 16 bytes (ROM grid stride)");

#define FIRERED_MAP_HEADER_SCALARS_MAPNUM_MAX 256u

static const FireredRomU32TableProfileRow s_map_header_scalars_profile_rows[] = {
    { 0u, "__firered_builtin_map_header_scalars_profile_none__", 0u },
};

extern const struct MapHeader *const *gMapGroups[];
extern const u8 gMapGroupSizes[];
extern const u32 gMapLayoutsSlotCount;

static const u8 *s_pack_base;
static u8 s_map_header_scalars_rom_active;

static size_t pack_byte_size(void)
{
    return (size_t)MAP_GROUPS_COUNT * (size_t)FIRERED_MAP_HEADER_SCALARS_MAPNUM_MAX * sizeof(FireredRomMapHeaderScalarsWire);
}

static const FireredRomMapHeaderScalarsWire *row_at(u16 mapGroup, u16 mapNum)
{
    size_t idx;

    if (!s_map_header_scalars_rom_active || s_pack_base == NULL)
        return NULL;
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= FIRERED_MAP_HEADER_SCALARS_MAPNUM_MAX)
        return NULL;
    idx = (size_t)mapGroup * (size_t)FIRERED_MAP_HEADER_SCALARS_MAPNUM_MAX + (size_t)mapNum;
    return (const FireredRomMapHeaderScalarsWire *)(s_pack_base + idx * sizeof(FireredRomMapHeaderScalarsWire));
}

static void clear_active(void)
{
    s_pack_base = NULL;
    s_map_header_scalars_rom_active = 0;
}

static bool8 row_is_zeroed(const FireredRomMapHeaderScalarsWire *row)
{
    static const FireredRomMapHeaderScalarsWire z;

    return (bool8)(memcmp(row, &z, sizeof(z)) == 0);
}

static bool8 validate_pack(size_t rom_size, const u8 *rom, size_t pack_off)
{
    u16 g;
    u16 n;

    if (pack_off > rom_size || pack_byte_size() > rom_size - pack_off)
        return FALSE;

    for (g = 0; g < (u16)MAP_GROUPS_COUNT; g++)
    {
        u8 maxn = gMapGroupSizes[g];

        for (n = 0; n < (u16)FIRERED_MAP_HEADER_SCALARS_MAPNUM_MAX; n++)
        {
            const FireredRomMapHeaderScalarsWire *row = (const FireredRomMapHeaderScalarsWire *)(rom + pack_off
                + ((size_t)g * (size_t)FIRERED_MAP_HEADER_SCALARS_MAPNUM_MAX + (size_t)n) * sizeof(FireredRomMapHeaderScalarsWire));

            if (n >= maxn)
            {
                if (!row_is_zeroed(row))
                    return FALSE;
                continue;
            }

            if (gMapGroups[g][n] == NULL)
                return FALSE;

            if (row->mapLayoutId == 0u || row->mapLayoutId > gMapLayoutsSlotCount)
                return FALSE;
            if (row->regionMapSectionId >= MAPSEC_COUNT)
                return FALSE;
            if (row->allowEscaping > 1u || row->allowRunning > 1u)
                return FALSE;
            if (row->showMapName > 63u)
                return FALSE;
        }
    }

    return TRUE;
}

static void apply_wire_to_header(struct MapHeader *dst, const FireredRomMapHeaderScalarsWire *w)
{
    dst->music = w->music;
    dst->mapLayoutId = w->mapLayoutId;
    dst->regionMapSectionId = w->regionMapSectionId;
    dst->cave = w->cave;
    dst->weather = w->weather;
    dst->mapType = w->mapType;
    dst->bikingAllowed = w->bikingAllowed;
    dst->allowEscaping = (bool8)(w->allowEscaping != 0);
    dst->allowRunning = (bool8)(w->allowRunning != 0);
    dst->showMapName = (bool8)(w->showMapName & 0x3Fu);
    dst->floorNum = w->floorNum;
    dst->battleType = w->battleType;
}

void firered_portable_rom_map_header_scalars_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;

    clear_active();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_HEADER_SCALARS_PACK_OFF",
            s_map_header_scalars_profile_rows, NELEMS(s_map_header_scalars_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (!validate_pack(rom_size, rom, pack_off))
        return;

    s_pack_base = rom + pack_off;
    s_map_header_scalars_rom_active = 1;
}

void firered_portable_rom_map_header_scalars_merge(struct MapHeader *dst, u16 mapGroup, u16 mapNum)
{
    const FireredRomMapHeaderScalarsWire *row;

    if (dst == NULL || !s_map_header_scalars_rom_active)
        return;
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= gMapGroupSizes[mapGroup])
        return;

    row = row_at(mapGroup, mapNum);
    if (row == NULL)
        return;

    apply_wire_to_header(dst, row);
}

u16 FireredRomMapHeaderScalarsEffectiveMapLayoutId(u16 mapGroup, u16 mapNum, u16 compiledLayoutId)
{
    const FireredRomMapHeaderScalarsWire *row;

    if (!s_map_header_scalars_rom_active)
        return compiledLayoutId;
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= gMapGroupSizes[mapGroup])
        return compiledLayoutId;

    row = row_at(mapGroup, mapNum);
    if (row == NULL)
        return compiledLayoutId;

    return row->mapLayoutId;
}

u16 FireredRomMapHeaderScalarsMusic(u16 mapGroup, u16 mapNum, u16 fallback)
{
    const FireredRomMapHeaderScalarsWire *row = row_at(mapGroup, mapNum);

    if (row == NULL || mapGroup >= MAP_GROUPS_COUNT || mapNum >= gMapGroupSizes[mapGroup])
        return fallback;
    return row->music;
}

u8 FireredRomMapHeaderScalarsRegionMapSec(u16 mapGroup, u16 mapNum, u8 fallback)
{
    const FireredRomMapHeaderScalarsWire *row = row_at(mapGroup, mapNum);

    if (row == NULL || mapGroup >= MAP_GROUPS_COUNT || mapNum >= gMapGroupSizes[mapGroup])
        return fallback;
    return row->regionMapSectionId;
}

u8 FireredRomMapHeaderScalarsMapType(u16 mapGroup, u16 mapNum, u8 fallback)
{
    const FireredRomMapHeaderScalarsWire *row = row_at(mapGroup, mapNum);

    if (row == NULL || mapGroup >= MAP_GROUPS_COUNT || mapNum >= gMapGroupSizes[mapGroup])
        return fallback;
    return row->mapType;
}

u8 FireredRomMapHeaderScalarsBattleType(u16 mapGroup, u16 mapNum, u8 fallback)
{
    const FireredRomMapHeaderScalarsWire *row = row_at(mapGroup, mapNum);

    if (row == NULL || mapGroup >= MAP_GROUPS_COUNT || mapNum >= gMapGroupSizes[mapGroup])
        return fallback;
    return row->battleType;
}

#endif /* PORTABLE */
