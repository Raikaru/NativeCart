#include "global.h"

#include "portable/firered_portable_rom_species_names_pack.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_species_names_pack_refresh_after_rom_load(void)
{
}

#else

#include "constants/global.h"
#include "constants/species.h"
#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

extern const u8 gSpeciesNames_Compiled[][POKEMON_NAME_LENGTH + 1];

/* Matches designated-init array in species_names_portable.h (0 .. SPECIES_CHIMECHO). */
#define FIRERED_SPECIES_NAMES_ROW_COUNT ((size_t)SPECIES_CHIMECHO + 1u)
#define FIRERED_SPECIES_NAMES_STRIDE ((size_t)POKEMON_NAME_LENGTH + 1u)
#define FIRERED_SPECIES_NAMES_PACK_BYTES (FIRERED_SPECIES_NAMES_ROW_COUNT * FIRERED_SPECIES_NAMES_STRIDE)

static const FireredRomU32TableProfileRow s_species_names_profile_rows[] = {
    { 0u, "__firered_builtin_species_names_profile_none__", 0u },
};

static u8 s_species_names_rom[FIRERED_SPECIES_NAMES_PACK_BYTES];
static u8 s_species_names_rom_active;

static const u8 (*s_species_names_active)[POKEMON_NAME_LENGTH + 1];

static void sync_active_ptr(void)
{
    s_species_names_active = s_species_names_rom_active ? (const u8 (*)[POKEMON_NAME_LENGTH + 1])s_species_names_rom : NULL;
}

const u8 (*FireredSpeciesNamesTable(void))[POKEMON_NAME_LENGTH + 1]
{
    if (s_species_names_active != NULL)
        return s_species_names_active;
    return gSpeciesNames_Compiled;
}

static bool8 row_has_eos(const u8 *row)
{
    size_t k;

    for (k = 0; k < FIRERED_SPECIES_NAMES_STRIDE; k++)
    {
        if (row[k] == (u8)0xFF)
            return TRUE;
    }
    return FALSE;
}

static bool8 species_names_rom_valid(const u8 *blob)
{
    size_t r;

    for (r = 0; r < FIRERED_SPECIES_NAMES_ROW_COUNT; r++)
    {
        if (!row_has_eos(blob + r * FIRERED_SPECIES_NAMES_STRIDE))
            return FALSE;
    }
    return TRUE;
}

void firered_portable_rom_species_names_pack_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;

    s_species_names_rom_active = 0;
    sync_active_ptr();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_SPECIES_NAMES_PACK_OFF",
            s_species_names_profile_rows, NELEMS(s_species_names_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (pack_off > rom_size || FIRERED_SPECIES_NAMES_PACK_BYTES > rom_size - pack_off)
        return;

    memcpy(s_species_names_rom, rom + pack_off, FIRERED_SPECIES_NAMES_PACK_BYTES);

    if (!species_names_rom_valid(s_species_names_rom))
        return;

    s_species_names_rom_active = 1;
    sync_active_ptr();
}

#endif /* PORTABLE */
