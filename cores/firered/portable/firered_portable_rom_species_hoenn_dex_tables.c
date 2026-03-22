#include "global.h"

#include "portable/firered_portable_rom_species_hoenn_dex_tables.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PORTABLE

void firered_portable_rom_species_hoenn_dex_tables_refresh_after_rom_load(void)
{
}

const u16 *firered_portable_rom_species_to_hoenn_dex_table(void)
{
    return NULL;
}

const u16 *firered_portable_rom_hoenn_to_national_order_table(void)
{
    return NULL;
}

#else

#include "constants/species.h"

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

static const FireredRomU32TableProfileRow s_species_to_hoenn_profile_rows[] = {
    { 0u, "__firered_builtin_species_to_hoenn_dex_profile_none__", 0u },
};

static const FireredRomU32TableProfileRow s_hoenn_to_national_order_profile_rows[] = {
    { 0u, "__firered_builtin_hoenn_to_national_order_profile_none__", 0u },
};

static u16 s_species_to_hoenn_rom[NUM_SPECIES - 1];
static u8 s_species_to_hoenn_rom_active;

static u16 s_hoenn_to_national_order_rom[NUM_SPECIES - 1];
static u8 s_hoenn_to_national_order_rom_active;

static void read_le_u16_block(const u8 *rom, size_t table_off, size_t word_count, u16 *out)
{
    size_t i;

    for (i = 0; i < word_count; i++)
    {
        size_t o = table_off + i * 2u;
        out[i] = (u16)rom[o] | ((u16)rom[o + 1u] << 8);
    }
}

static void try_load_one_table(const char *env_name, const FireredRomU32TableProfileRow *rows, size_t row_count,
    u16 *out_buf, u8 *out_active)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;
    size_t n;

    *out_active = 0;

    n = (size_t)(NUM_SPECIES - 1);
    if (n == 0u)
        return;

    need = n * 2u;
    if (!firered_portable_rom_u32_table_resolve_base_off(env_name, rows, row_count, &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    read_le_u16_block(rom, table_off, n, out_buf);
    *out_active = 1;
}

void firered_portable_rom_species_hoenn_dex_tables_refresh_after_rom_load(void)
{
    s_species_to_hoenn_rom_active = 0;
    s_hoenn_to_national_order_rom_active = 0;

    try_load_one_table("FIRERED_ROM_SPECIES_TO_HOENN_DEX_OFF", s_species_to_hoenn_profile_rows,
        NELEMS(s_species_to_hoenn_profile_rows), s_species_to_hoenn_rom, &s_species_to_hoenn_rom_active);

    try_load_one_table("FIRERED_ROM_HOENN_TO_NATIONAL_ORDER_OFF", s_hoenn_to_national_order_profile_rows,
        NELEMS(s_hoenn_to_national_order_profile_rows), s_hoenn_to_national_order_rom,
        &s_hoenn_to_national_order_rom_active);
}

const u16 *firered_portable_rom_species_to_hoenn_dex_table(void)
{
    if (!s_species_to_hoenn_rom_active)
        return NULL;
    return s_species_to_hoenn_rom;
}

const u16 *firered_portable_rom_hoenn_to_national_order_table(void)
{
    if (!s_hoenn_to_national_order_rom_active)
        return NULL;
    return s_hoenn_to_national_order_rom;
}

#endif /* PORTABLE */
