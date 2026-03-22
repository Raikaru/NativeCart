#include "global.h"

#include "portable/firered_portable_rom_heal_locations_table.h"

#include "constants/heal_locations.h"
#include "heal_location.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_heal_locations_table_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

#define FIRERED_HEAL_LOCATION_ROW_BYTES ((size_t)sizeof(struct HealLocation))
#define FIRERED_HEAL_LOCATIONS_TABLE_BYTE_SIZE ((size_t)HEAL_LOCATION_COUNT * FIRERED_HEAL_LOCATION_ROW_BYTES)

#define FIRERED_WHITEOUT_MAP_IDX_ROW_BYTES ((size_t)(sizeof(u16) * 2u))
#define FIRERED_WHITEOUT_MAP_IDXS_BYTE_SIZE ((size_t)HEAL_LOCATION_COUNT * FIRERED_WHITEOUT_MAP_IDX_ROW_BYTES)
#define FIRERED_WHITEOUT_NPC_IDS_BYTE_SIZE ((size_t)HEAL_LOCATION_COUNT * sizeof(u8))

static const FireredRomU32TableProfileRow s_heal_locations_profile_rows[] = {
    { 0u, "__firered_builtin_heal_locations_profile_none__", 0u },
};

static const FireredRomU32TableProfileRow s_whiteout_map_idxs_profile_rows[] = {
    { 0u, "__firered_builtin_heal_locations_profile_none__", 0u },
};

static const FireredRomU32TableProfileRow s_whiteout_npc_ids_profile_rows[] = {
    { 0u, "__firered_builtin_heal_locations_profile_none__", 0u },
};

static struct HealLocation s_heal_locations_rom[HEAL_LOCATION_COUNT];
static u8 s_heal_locations_rom_active;

static u16 s_whiteout_map_idxs_rom[HEAL_LOCATION_COUNT][2];
static u8 s_whiteout_npc_ids_rom[HEAL_LOCATION_COUNT];
static u8 s_whiteout_companion_rom_active;

const struct HealLocation *gHealLocationsActive;
const u16 (*gWhiteoutRespawnHealCenterMapIdxsActive)[2];
const u8 *gWhiteoutRespawnHealerNpcIdsActive;

_Static_assert(FIRERED_HEAL_LOCATIONS_TABLE_BYTE_SIZE == sizeof(s_heal_locations_rom), "heal locations cache size");
_Static_assert(FIRERED_WHITEOUT_MAP_IDXS_BYTE_SIZE == sizeof(s_whiteout_map_idxs_rom), "whiteout map idx cache size");
_Static_assert(FIRERED_WHITEOUT_NPC_IDS_BYTE_SIZE == sizeof(s_whiteout_npc_ids_rom), "whiteout npc id cache size");

static void heal_locations_sync_active_ptr(void)
{
    gHealLocationsActive = s_heal_locations_rom_active ? s_heal_locations_rom : NULL;
}

static void whiteout_companion_sync_active_ptrs(void)
{
    if (s_whiteout_companion_rom_active)
    {
        gWhiteoutRespawnHealCenterMapIdxsActive = s_whiteout_map_idxs_rom;
        gWhiteoutRespawnHealerNpcIdsActive = s_whiteout_npc_ids_rom;
    }
    else
    {
        gWhiteoutRespawnHealCenterMapIdxsActive = NULL;
        gWhiteoutRespawnHealerNpcIdsActive = NULL;
    }
}

/* Light sanity; map group/num are engine-defined. */
static int heal_locations_rom_valid(const struct HealLocation *tbl)
{
    size_t i;

    for (i = 0; i < HEAL_LOCATION_COUNT; i++)
    {
        const struct HealLocation *h = &tbl[i];

        if (h->mapGroup < 0 || h->mapGroup > 50)
            return 0;
        if (h->mapNum < 0 || h->mapNum > 120)
            return 0;
        if (h->x < -500 || h->x > 500 || h->y < -500 || h->y > 500)
            return 0;
    }
    return 1;
}

/* Same map idx ranges as heal blob; wire is u16[2] per row. */
static int whiteout_map_idxs_rom_valid(const u16 (*tbl)[2])
{
    size_t i;

    for (i = 0; i < HEAL_LOCATION_COUNT; i++)
    {
        int g = (int)tbl[i][0];
        int n = (int)tbl[i][1];

        if (g < 0 || g > 50)
            return 0;
        if (n < 0 || n > 120)
            return 0;
    }
    return 1;
}

static int whiteout_npc_ids_rom_valid(const u8 *tbl)
{
    size_t i;

    for (i = 0; i < HEAL_LOCATION_COUNT; i++)
    {
        if (tbl[i] == 0)
            return 0;
    }
    return 1;
}

void firered_portable_rom_heal_locations_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;
    size_t whiteout_maps_off;
    size_t whiteout_npcs_off;

    s_heal_locations_rom_active = 0;
    heal_locations_sync_active_ptr();
    s_whiteout_companion_rom_active = 0;
    whiteout_companion_sync_active_ptrs();

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;

    /* Heal / fly table (independent of whiteout companions). */
    need = FIRERED_HEAL_LOCATIONS_TABLE_BYTE_SIZE;
    if (firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_HEAL_LOCATIONS_TABLE_OFF",
            s_heal_locations_profile_rows, NELEMS(s_heal_locations_profile_rows), &table_off)
        && rom_size >= FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES && rom != NULL && table_off <= rom_size
        && need <= rom_size - table_off)
    {
        memcpy(s_heal_locations_rom, rom + table_off, need);
        if (heal_locations_rom_valid(s_heal_locations_rom))
            s_heal_locations_rom_active = 1;
    }
    heal_locations_sync_active_ptr();

    /*
     * Whiteout companions: **both** blobs must resolve, fit in ROM, validate, or **neither** is used
     * (compiled `*_Compiled` via heal_location.c macros). Independent of heal blob success.
     */
    if (firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_WHITEOUT_HEAL_CENTER_MAP_IDXS_OFF",
            s_whiteout_map_idxs_profile_rows, NELEMS(s_whiteout_map_idxs_profile_rows), &whiteout_maps_off)
        && firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_WHITEOUT_HEALER_NPC_IDS_OFF",
            s_whiteout_npc_ids_profile_rows, NELEMS(s_whiteout_npc_ids_profile_rows), &whiteout_npcs_off)
        && rom_size >= FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES && rom != NULL
        && whiteout_maps_off <= rom_size
        && FIRERED_WHITEOUT_MAP_IDXS_BYTE_SIZE <= rom_size - whiteout_maps_off
        && whiteout_npcs_off <= rom_size
        && FIRERED_WHITEOUT_NPC_IDS_BYTE_SIZE <= rom_size - whiteout_npcs_off)
    {
        memcpy(s_whiteout_map_idxs_rom, rom + whiteout_maps_off, FIRERED_WHITEOUT_MAP_IDXS_BYTE_SIZE);
        memcpy(s_whiteout_npc_ids_rom, rom + whiteout_npcs_off, FIRERED_WHITEOUT_NPC_IDS_BYTE_SIZE);
        if (whiteout_map_idxs_rom_valid(s_whiteout_map_idxs_rom)
            && whiteout_npc_ids_rom_valid(s_whiteout_npc_ids_rom))
            s_whiteout_companion_rom_active = 1;
    }
    whiteout_companion_sync_active_ptrs();
}

#endif /* PORTABLE */
