#include "global.h"

#include "portable/firered_portable_rom_hm_moves_table.h"

#include <stddef.h>

#ifndef PORTABLE

void firered_portable_rom_hm_moves_table_refresh_after_rom_load(void)
{
}

const u16 *firered_portable_rom_hm_moves_table(void)
{
    return NULL;
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

#define HM_MOVES_END_VALUE 0xFFFFu

static const FireredRomU32TableProfileRow s_hm_moves_profile_rows[] = {
    { 0u, "__firered_builtin_hm_moves_profile_none__", 0u },
};

static u16 s_hm_moves_rom[FIRERED_ROM_HM_MOVES_TABLE_U16_COUNT];
static u8 s_hm_moves_rom_active;

static void read_le_u16_block(const u8 *rom, size_t table_off, size_t word_count, u16 *out)
{
    size_t i;

    for (i = 0; i < word_count; i++)
    {
        size_t o = table_off + i * 2u;
        out[i] = (u16)rom[o] | ((u16)rom[o + 1u] << 8);
    }
}

static int hm_moves_block_has_sentinel(const u16 *words, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
    {
        if (words[i] == (u16)HM_MOVES_END_VALUE)
            return 1;
    }

    return 0;
}

void firered_portable_rom_hm_moves_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;

    s_hm_moves_rom_active = 0;

    need = FIRERED_ROM_HM_MOVES_TABLE_U16_COUNT * 2u;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_HM_MOVES_TABLE_OFF",
            s_hm_moves_profile_rows, NELEMS(s_hm_moves_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    read_le_u16_block(rom, table_off, FIRERED_ROM_HM_MOVES_TABLE_U16_COUNT, s_hm_moves_rom);
    if (!hm_moves_block_has_sentinel(s_hm_moves_rom, FIRERED_ROM_HM_MOVES_TABLE_U16_COUNT))
        return;

    s_hm_moves_rom_active = 1;
}

const u16 *firered_portable_rom_hm_moves_table(void)
{
    if (!s_hm_moves_rom_active)
        return NULL;
    return s_hm_moves_rom;
}

#endif /* PORTABLE */
