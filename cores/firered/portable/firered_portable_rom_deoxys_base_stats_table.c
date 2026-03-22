#include "global.h"

#include "portable/firered_portable_rom_deoxys_base_stats_table.h"

#include <stddef.h>

#ifndef PORTABLE

void firered_portable_rom_deoxys_base_stats_table_refresh_after_rom_load(void)
{
}

const u16 *firered_portable_rom_deoxys_base_stats_table(void)
{
    return NULL;
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

static const FireredRomU32TableProfileRow s_deoxys_base_stats_profile_rows[] = {
    { 0u, "__firered_builtin_deoxys_base_stats_profile_none__", 0u },
};

static u16 s_deoxys_base_stats_rom[FIRERED_ROM_DEOXYS_BASE_STATS_TABLE_U16_COUNT];
static u8 s_deoxys_base_stats_rom_active;

static void read_le_u16_block(const u8 *rom, size_t table_off, size_t word_count, u16 *out)
{
    size_t i;

    for (i = 0; i < word_count; i++)
    {
        size_t o = table_off + i * 2u;
        out[i] = (u16)rom[o] | ((u16)rom[o + 1u] << 8);
    }
}

void firered_portable_rom_deoxys_base_stats_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;
    size_t n;

    s_deoxys_base_stats_rom_active = 0;

    n = FIRERED_ROM_DEOXYS_BASE_STATS_TABLE_U16_COUNT;
    if (n == 0u)
        return;

    need = n * 2u;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_DEOXYS_BASE_STATS_TABLE_OFF",
            s_deoxys_base_stats_profile_rows, NELEMS(s_deoxys_base_stats_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    read_le_u16_block(rom, table_off, n, s_deoxys_base_stats_rom);
    s_deoxys_base_stats_rom_active = 1;
}

const u16 *firered_portable_rom_deoxys_base_stats_table(void)
{
    if (!s_deoxys_base_stats_rom_active)
        return NULL;
    return s_deoxys_base_stats_rom;
}

#endif /* PORTABLE */
