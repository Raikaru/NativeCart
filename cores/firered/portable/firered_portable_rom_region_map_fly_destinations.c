#include "global.h"

#include "portable/firered_portable_rom_region_map_fly_destinations.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_region_map_fly_destinations_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/heal_locations.h"
#include "constants/region_map_fly_destinations.h"
#include "engine_internal.h"
#include "region_map_fly_destinations_rom.h"

#define FIRERED_REGION_MAP_FLY_DESTINATIONS_BYTE_SIZE \
    ((size_t)REGION_MAP_FLY_DESTINATION_ROW_COUNT * 3u)

static const FireredRomU32TableProfileRow s_region_map_fly_destinations_profile_rows[] = {
    { 0u, "__firered_builtin_region_map_fly_destinations_profile_none__", 0u },
};

static u8 s_map_fly_destinations_rom[REGION_MAP_FLY_DESTINATION_ROW_COUNT][3];
static u8 s_region_map_fly_destinations_rom_active;

const u8 (*gMapFlyDestinationsActive)[3];

_Static_assert(sizeof(s_map_fly_destinations_rom) == FIRERED_REGION_MAP_FLY_DESTINATIONS_BYTE_SIZE, "fly table");

static void region_map_fly_destinations_sync_active_ptr(void)
{
    gMapFlyDestinationsActive = s_region_map_fly_destinations_rom_active ? s_map_fly_destinations_rom : NULL;
}

/* Same map idx ranges as heal / whiteout portable validators. */
static int region_map_fly_destinations_rom_valid(const u8 (*tbl)[3])
{
    size_t i;

    for (i = 0; i < (size_t)REGION_MAP_FLY_DESTINATION_ROW_COUNT; i++)
    {
        int g = (int)tbl[i][0];
        int n = (int)tbl[i][1];
        u8 heal = tbl[i][2];

        if (g < 0 || g > 50)
            return 0;
        if (n < 0 || n > 120)
            return 0;
        if (heal > HEAL_LOCATION_COUNT)
            return 0;
    }
    return 1;
}

void firered_portable_rom_region_map_fly_destinations_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;

    s_region_map_fly_destinations_rom_active = 0;
    region_map_fly_destinations_sync_active_ptr();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_REGION_MAP_FLY_DESTINATIONS_TABLE_OFF",
            s_region_map_fly_destinations_profile_rows, NELEMS(s_region_map_fly_destinations_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || FIRERED_REGION_MAP_FLY_DESTINATIONS_BYTE_SIZE > rom_size - table_off)
        return;

    memcpy(s_map_fly_destinations_rom, rom + table_off, FIRERED_REGION_MAP_FLY_DESTINATIONS_BYTE_SIZE);
    if (!region_map_fly_destinations_rom_valid(s_map_fly_destinations_rom))
        return;

    s_region_map_fly_destinations_rom_active = 1;
    region_map_fly_destinations_sync_active_ptr();
}

#endif /* PORTABLE */
