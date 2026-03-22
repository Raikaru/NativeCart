#include "global.h"

#include "portable/firered_portable_rom_region_map_section_layout.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_region_map_section_layout_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/region_map_layout_dims.h"
#include "constants/region_map_sections.h"
#include "engine_internal.h"
#include "region_map_section_layout_rom.h"

#define FIRERED_REGION_MAP_LAYOUT_ONE_TABLE_BYTES ((size_t)MAPSEC_COUNT * sizeof(u16) * 2u)
#define FIRERED_REGION_MAP_LAYOUT_PACK_BYTE_SIZE (FIRERED_REGION_MAP_LAYOUT_ONE_TABLE_BYTES * 2u)

static const FireredRomU32TableProfileRow s_region_map_layout_profile_rows[] = {
    { 0u, "__firered_builtin_region_map_section_layout_profile_none__", 0u },
};

static u16 s_map_section_top_left_rom[MAPSEC_COUNT][2];
static u16 s_map_section_dimensions_rom[MAPSEC_COUNT][2];
static u8 s_region_map_layout_rom_active;

const u16 (*gMapSectionTopLeftCornersActive)[2];
const u16 (*gMapSectionDimensionsActive)[2];

_Static_assert(sizeof(s_map_section_top_left_rom) == FIRERED_REGION_MAP_LAYOUT_ONE_TABLE_BYTES, "corners size");
_Static_assert(sizeof(s_map_section_dimensions_rom) == FIRERED_REGION_MAP_LAYOUT_ONE_TABLE_BYTES, "dims size");

static void region_map_layout_sync_active_ptrs(void)
{
    if (s_region_map_layout_rom_active)
    {
        gMapSectionTopLeftCornersActive = s_map_section_top_left_rom;
        gMapSectionDimensionsActive = s_map_section_dimensions_rom;
    }
    else
    {
        gMapSectionTopLeftCornersActive = NULL;
        gMapSectionDimensionsActive = NULL;
    }
}

static int region_map_layout_corners_valid(const u16 (*tbl)[2])
{
    size_t i;

    for (i = 0; i < (size_t)MAPSEC_COUNT; i++)
    {
        u16 x = tbl[i][0];
        u16 y = tbl[i][1];

        if ((unsigned)x >= (unsigned)REGION_MAP_SECTION_GRID_WIDTH
            || (unsigned)y >= (unsigned)REGION_MAP_SECTION_GRID_HEIGHT)
            return 0;
    }
    return 1;
}

static int region_map_layout_dimensions_valid(const u16 (*tbl)[2])
{
    size_t i;

    for (i = 0; i < (size_t)MAPSEC_COUNT; i++)
    {
        u16 w = tbl[i][0];
        u16 h = tbl[i][1];

        /* Generous cap; vanilla uses small integers. SPECIAL_AREA uses {0,0}. */
        if ((unsigned)w > 32u || (unsigned)h > 32u)
            return 0;
    }
    return 1;
}

void firered_portable_rom_region_map_section_layout_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;

    s_region_map_layout_rom_active = 0;
    region_map_layout_sync_active_ptrs();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_REGION_MAP_SECTION_LAYOUT_PACK_OFF",
            s_region_map_layout_profile_rows, NELEMS(s_region_map_layout_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (pack_off > rom_size || FIRERED_REGION_MAP_LAYOUT_PACK_BYTE_SIZE > rom_size - pack_off)
        return;

    memcpy(s_map_section_top_left_rom, rom + pack_off, FIRERED_REGION_MAP_LAYOUT_ONE_TABLE_BYTES);
    memcpy(s_map_section_dimensions_rom, rom + pack_off + FIRERED_REGION_MAP_LAYOUT_ONE_TABLE_BYTES,
        FIRERED_REGION_MAP_LAYOUT_ONE_TABLE_BYTES);

    if (!region_map_layout_corners_valid(s_map_section_top_left_rom)
        || !region_map_layout_dimensions_valid(s_map_section_dimensions_rom))
        return;

    s_region_map_layout_rom_active = 1;
    region_map_layout_sync_active_ptrs();
}

#endif /* PORTABLE */
