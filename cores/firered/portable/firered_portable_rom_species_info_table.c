#include "global.h"

#include "portable/firered_portable_rom_species_info_table.h"

#include "pokemon.h"

#include "constants/pokemon.h"
#include "constants/species.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_species_info_table_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

/* Wire layout matches pokefirered struct SpeciesInfo on host (GCC): sizeof = 26. */
#define FIRERED_SPECIES_INFO_WIRE_BYTES ((size_t)26)
#define FIRERED_SPECIES_INFO_ROM_ENTRY_COUNT ((size_t)NUM_SPECIES)

static const FireredRomU32TableProfileRow s_species_info_profile_rows[] = {
    { 0u, "__firered_builtin_species_info_profile_none__", 0u },
};

static struct SpeciesInfo s_species_info_rom[FIRERED_SPECIES_INFO_ROM_ENTRY_COUNT];
static u8 s_species_info_rom_active;

extern const struct SpeciesInfo gSpeciesInfo_Compiled[];

/* NULL => use compiled gSpeciesInfo_Compiled (see pokemon.h macro). */
const struct SpeciesInfo *gSpeciesInfoActive;

static void species_info_sync_read_ptr(void)
{
    gSpeciesInfoActive = s_species_info_rom_active ? s_species_info_rom : NULL;
}

static void species_info_unpack_wire_row(const u8 *p, struct SpeciesInfo *out)
{
    u16 evPack;
    u8 last;

    out->baseHP = p[0];
    out->baseAttack = p[1];
    out->baseDefense = p[2];
    out->baseSpeed = p[3];
    out->baseSpAttack = p[4];
    out->baseSpDefense = p[5];
    out->types[0] = p[6];
    out->types[1] = p[7];
    out->catchRate = p[8];
    out->expYield = p[9];
    evPack = (u16)p[10] | ((u16)p[11] << 8);
    out->evYield_HP = evPack & 3u;
    out->evYield_Attack = (evPack >> 2) & 3u;
    out->evYield_Defense = (evPack >> 4) & 3u;
    out->evYield_Speed = (evPack >> 6) & 3u;
    out->evYield_SpAttack = (evPack >> 8) & 3u;
    out->evYield_SpDefense = (evPack >> 10) & 3u;
    out->itemCommon = (u16)p[12] | ((u16)p[13] << 8);
    out->itemRare = (u16)p[14] | ((u16)p[15] << 8);
    out->genderRatio = p[16];
    out->eggCycles = p[17];
    out->friendship = p[18];
    out->growthRate = p[19];
    out->eggGroups[0] = p[20];
    out->eggGroups[1] = p[21];
    out->abilities[0] = p[22];
    out->abilities[1] = p[23];
    out->safariZoneFleeRate = p[24];
    last = p[25];
    out->bodyColor = last & 0x7Fu;
    out->noFlip = (u8)((last >> 7) & 1u);
}

static int species_info_unpack_selftest(void)
{
    struct SpeciesInfo u;

    if (sizeof(struct SpeciesInfo) != FIRERED_SPECIES_INFO_WIRE_BYTES)
        return 0;

    species_info_unpack_wire_row((const u8 *)&gSpeciesInfo_Compiled[SPECIES_BULBASAUR], &u);
    return memcmp(&u, &gSpeciesInfo_Compiled[SPECIES_BULBASAUR], sizeof u) == 0;
}

static int species_info_row_structurally_ok(const struct SpeciesInfo *r)
{
    if ((u32)r->types[0] >= (u32)NUMBER_OF_MON_TYPES || (u32)r->types[1] >= (u32)NUMBER_OF_MON_TYPES)
        return 0;
    if (r->growthRate > GROWTH_SLOW)
        return 0;
    return 1;
}

void firered_portable_rom_species_info_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;
    size_t row;
    const u8 *src;

    s_species_info_rom_active = 0;
    species_info_sync_read_ptr();

    if (!species_info_unpack_selftest())
        return;

    need = FIRERED_SPECIES_INFO_ROM_ENTRY_COUNT * FIRERED_SPECIES_INFO_WIRE_BYTES;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_SPECIES_INFO_TABLE_OFF",
            s_species_info_profile_rows, NELEMS(s_species_info_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    src = rom + table_off;
    for (row = 0; row < FIRERED_SPECIES_INFO_ROM_ENTRY_COUNT; row++)
    {
        species_info_unpack_wire_row(src + row * FIRERED_SPECIES_INFO_WIRE_BYTES, &s_species_info_rom[row]);
        if (!species_info_row_structurally_ok(&s_species_info_rom[row]))
            return;
    }

    s_species_info_rom_active = 1;
    species_info_sync_read_ptr();
}

#endif /* PORTABLE */
