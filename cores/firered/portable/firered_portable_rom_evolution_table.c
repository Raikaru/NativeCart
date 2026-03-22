#include "global.h"

#include "portable/firered_portable_rom_evolution_table.h"

#include "constants/pokemon.h"
#include "constants/species.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_evolution_table_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

#define FIRERED_EVOLUTION_ROM_ROW_BYTES ((size_t)6)
#define FIRERED_EVOLUTION_ROM_SPECIES_ROWS ((size_t)NUM_SPECIES)
#define FIRERED_EVOLUTION_ROM_SLOTS ((size_t)EVOS_PER_MON)
#define FIRERED_EVOLUTION_TABLE_BYTE_SIZE \
    (FIRERED_EVOLUTION_ROM_SPECIES_ROWS * FIRERED_EVOLUTION_ROM_SLOTS * FIRERED_EVOLUTION_ROM_ROW_BYTES)

static const FireredRomU32TableProfileRow s_evolution_profile_rows[] = {
    { 0u, "__firered_builtin_evolution_profile_none__", 0u },
};

static struct Evolution s_evolution_rom[NUM_SPECIES][EVOS_PER_MON];
static u8 s_evolution_rom_active;

const struct Evolution (*gEvolutionTableActive)[EVOS_PER_MON];

_Static_assert(sizeof(struct Evolution) == FIRERED_EVOLUTION_ROM_ROW_BYTES, "portable Evolution wire size");
_Static_assert(FIRERED_EVOLUTION_TABLE_BYTE_SIZE == sizeof(s_evolution_rom), "evolution cache size");

static void evolution_sync_active_ptr(void)
{
    gEvolutionTableActive = s_evolution_rom_active
        ? (const struct Evolution (*)[EVOS_PER_MON])s_evolution_rom
        : NULL;
}

/*
 * Structural validation only (no equality vs compiled — hacks may change evolutions).
 * - SPECIES_NONE: all slots must be empty (method/param/target 0).
 * - method == 0 => param and targetSpecies must be 0 (padding / unused slot).
 * - method != 0 => vanilla FireRed evolution method range + plausible target species index.
 */
static int evolution_rom_valid(const struct Evolution (*tbl)[EVOS_PER_MON])
{
    size_t s, j;

    for (j = 0; j < EVOS_PER_MON; j++)
    {
        if (tbl[SPECIES_NONE][j].method != 0
         || tbl[SPECIES_NONE][j].param != 0
         || tbl[SPECIES_NONE][j].targetSpecies != 0)
            return 0;
    }

    for (s = 0; s < NUM_SPECIES; s++)
    {
        for (j = 0; j < EVOS_PER_MON; j++)
        {
            struct Evolution e = tbl[s][j];

            if (e.method == 0)
            {
                if (e.param != 0 || e.targetSpecies != 0)
                    return 0;
            }
            else
            {
                if (e.method < EVO_FRIENDSHIP || e.method > EVO_BEAUTY)
                    return 0;
                if (e.targetSpecies == SPECIES_NONE || e.targetSpecies >= NUM_SPECIES)
                    return 0;
            }
        }
    }

    return 1;
}

void firered_portable_rom_evolution_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;

    s_evolution_rom_active = 0;
    evolution_sync_active_ptr();

    need = FIRERED_EVOLUTION_TABLE_BYTE_SIZE;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_EVOLUTION_TABLE_OFF",
            s_evolution_profile_rows, NELEMS(s_evolution_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    memcpy(s_evolution_rom, rom + table_off, need);
    if (!evolution_rom_valid((const struct Evolution (*)[EVOS_PER_MON])s_evolution_rom))
        return;

    s_evolution_rom_active = 1;
    evolution_sync_active_ptr();
}

#endif /* PORTABLE */
