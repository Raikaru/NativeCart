#include "global.h"

#include "portable/firered_portable_rom_region_map_section_grids.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_region_map_section_grids_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/region_map_layout_dims.h"
#include "constants/region_map_sections.h"
#include "engine_internal.h"
#include "region_map_section_grids_rom.h"

#define FIRERED_REGION_MAP_SECTION_GRIDS_PACK_GRID_COUNT 4u

#define FIRERED_REGION_MAP_SECTION_GRIDS_PACK_BYTE_SIZE \
    ((size_t)REGION_MAP_SECTION_GRID_BYTE_SIZE * (size_t)FIRERED_REGION_MAP_SECTION_GRIDS_PACK_GRID_COUNT)

static const FireredRomU32TableProfileRow s_region_map_section_grids_profile_rows[] = {
    { 0u, "__firered_builtin_region_map_section_grids_profile_none__", 0u },
};

static u8 s_region_map_grid_kanto_rom[REGION_MAP_SECTION_GRID_LAYERS][REGION_MAP_SECTION_GRID_HEIGHT]
                                      [REGION_MAP_SECTION_GRID_WIDTH];
static u8 s_region_map_grid_sevii123_rom[REGION_MAP_SECTION_GRID_LAYERS][REGION_MAP_SECTION_GRID_HEIGHT]
                                        [REGION_MAP_SECTION_GRID_WIDTH];
static u8 s_region_map_grid_sevii45_rom[REGION_MAP_SECTION_GRID_LAYERS][REGION_MAP_SECTION_GRID_HEIGHT]
                                       [REGION_MAP_SECTION_GRID_WIDTH];
static u8 s_region_map_grid_sevii67_rom[REGION_MAP_SECTION_GRID_LAYERS][REGION_MAP_SECTION_GRID_HEIGHT]
                                       [REGION_MAP_SECTION_GRID_WIDTH];

static u8 s_region_map_section_grids_rom_active;

const u8 (*gRegionMapSectionGridKantoActive)[REGION_MAP_SECTION_GRID_HEIGHT][REGION_MAP_SECTION_GRID_WIDTH];
const u8 (*gRegionMapSectionGridSevii123Active)[REGION_MAP_SECTION_GRID_HEIGHT][REGION_MAP_SECTION_GRID_WIDTH];
const u8 (*gRegionMapSectionGridSevii45Active)[REGION_MAP_SECTION_GRID_HEIGHT][REGION_MAP_SECTION_GRID_WIDTH];
const u8 (*gRegionMapSectionGridSevii67Active)[REGION_MAP_SECTION_GRID_HEIGHT][REGION_MAP_SECTION_GRID_WIDTH];

_Static_assert(sizeof(s_region_map_grid_kanto_rom) == REGION_MAP_SECTION_GRID_BYTE_SIZE, "kanto grid size");
_Static_assert(FIRERED_REGION_MAP_SECTION_GRIDS_PACK_BYTE_SIZE
        == (size_t)REGION_MAP_SECTION_GRID_BYTE_SIZE * FIRERED_REGION_MAP_SECTION_GRIDS_PACK_GRID_COUNT,
    "grids pack size");

static void region_map_section_grids_sync_active_ptrs(void)
{
    if (s_region_map_section_grids_rom_active)
    {
        gRegionMapSectionGridKantoActive = s_region_map_grid_kanto_rom;
        gRegionMapSectionGridSevii123Active = s_region_map_grid_sevii123_rom;
        gRegionMapSectionGridSevii45Active = s_region_map_grid_sevii45_rom;
        gRegionMapSectionGridSevii67Active = s_region_map_grid_sevii67_rom;
    }
    else
    {
        gRegionMapSectionGridKantoActive = NULL;
        gRegionMapSectionGridSevii123Active = NULL;
        gRegionMapSectionGridSevii45Active = NULL;
        gRegionMapSectionGridSevii67Active = NULL;
    }
}

static int region_map_section_grid_bytes_valid(const u8 *bytes, size_t byte_count)
{
    size_t i;

    for (i = 0; i < byte_count; i++)
    {
        u8 b = bytes[i];

        if ((unsigned)b >= (unsigned)MAPSEC_COUNT)
            return 0;
    }
    return 1;
}

void firered_portable_rom_region_map_section_grids_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;

    s_region_map_section_grids_rom_active = 0;
    region_map_section_grids_sync_active_ptrs();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_REGION_MAP_SECTION_GRIDS_PACK_OFF",
            s_region_map_section_grids_profile_rows, NELEMS(s_region_map_section_grids_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (pack_off > rom_size || FIRERED_REGION_MAP_SECTION_GRIDS_PACK_BYTE_SIZE > rom_size - pack_off)
        return;

    memcpy(s_region_map_grid_kanto_rom, rom + pack_off, (size_t)REGION_MAP_SECTION_GRID_BYTE_SIZE);
    memcpy(s_region_map_grid_sevii123_rom, rom + pack_off + (size_t)REGION_MAP_SECTION_GRID_BYTE_SIZE,
        (size_t)REGION_MAP_SECTION_GRID_BYTE_SIZE);
    memcpy(s_region_map_grid_sevii45_rom, rom + pack_off + (size_t)REGION_MAP_SECTION_GRID_BYTE_SIZE * 2u,
        (size_t)REGION_MAP_SECTION_GRID_BYTE_SIZE);
    memcpy(s_region_map_grid_sevii67_rom, rom + pack_off + (size_t)REGION_MAP_SECTION_GRID_BYTE_SIZE * 3u,
        (size_t)REGION_MAP_SECTION_GRID_BYTE_SIZE);

    if (!region_map_section_grid_bytes_valid((const u8 *)s_region_map_grid_kanto_rom, sizeof(s_region_map_grid_kanto_rom))
        || !region_map_section_grid_bytes_valid((const u8 *)s_region_map_grid_sevii123_rom,
            sizeof(s_region_map_grid_sevii123_rom))
        || !region_map_section_grid_bytes_valid((const u8 *)s_region_map_grid_sevii45_rom,
            sizeof(s_region_map_grid_sevii45_rom))
        || !region_map_section_grid_bytes_valid((const u8 *)s_region_map_grid_sevii67_rom,
            sizeof(s_region_map_grid_sevii67_rom)))
        return;

    s_region_map_section_grids_rom_active = 1;
    region_map_section_grids_sync_active_ptrs();
}

#endif /* PORTABLE */
