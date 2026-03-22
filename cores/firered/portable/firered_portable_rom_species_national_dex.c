#include "global.h"

#include "portable/firered_portable_rom_species_national_dex.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PORTABLE

void firered_portable_rom_species_national_dex_refresh_after_rom_load(void)
{
}

const u16 *firered_portable_rom_species_national_dex_table(void)
{
    return NULL;
}

#else

#include "constants/species.h"

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

static const FireredRomU32TableProfileRow s_species_national_dex_profile_rows[] = {
    { 0u, "__firered_builtin_species_national_dex_profile_none__", 0u },
};

static u16 s_species_to_national_rom[NUM_SPECIES - 1];
static u8 s_species_to_national_rom_active;

static void read_le_u16_block(const u8 *rom, size_t table_off, size_t word_count, u16 *out)
{
    size_t i;

    for (i = 0; i < word_count; i++)
    {
        size_t o = table_off + i * 2u;
        out[i] = (u16)rom[o] | ((u16)rom[o + 1u] << 8);
    }
}

void firered_portable_rom_species_national_dex_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;
    size_t n;

    s_species_to_national_rom_active = 0;

    n = (size_t)(NUM_SPECIES - 1);
    if (n == 0u)
        return;

    need = n * 2u;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_SPECIES_TO_NATIONAL_DEX_OFF",
            s_species_national_dex_profile_rows, NELEMS(s_species_national_dex_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    read_le_u16_block(rom, table_off, n, s_species_to_national_rom);
    s_species_to_national_rom_active = 1;
}

const u16 *firered_portable_rom_species_national_dex_table(void)
{
    if (!s_species_to_national_rom_active)
        return NULL;
    return s_species_to_national_rom;
}

#endif /* PORTABLE */
