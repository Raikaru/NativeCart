#include "global.h"

#include "portable/firered_portable_rom_battle_terrain_tables.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_battle_terrain_tables_refresh_after_rom_load(void)
{
}

const u16 *firered_portable_rom_battle_terrain_nature_power_moves(void)
{
    return NULL;
}

const u8 *firered_portable_rom_battle_terrain_to_type(void)
{
    return NULL;
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/moves.h"
#include "constants/pokemon.h"
#include "engine_internal.h"

static const FireredRomU32TableProfileRow s_battle_terrain_tables_profile_rows[] = {
    { 0u, "__firered_builtin_battle_terrain_tables_profile_none__", 0u },
};

static u16 s_nature_power_moves_rom[NATURE_POWER_MOVES_ROW_COUNT];
static u8 s_terrain_to_type_rom[NATURE_POWER_MOVES_ROW_COUNT];
static u8 s_battle_terrain_tables_rom_active;

static void read_le_u16_block(const u8 *rom, size_t table_off, size_t word_count, u16 *out)
{
    size_t i;

    for (i = 0; i < word_count; i++)
    {
        size_t o = table_off + i * 2u;

        out[i] = (u16)rom[o] | ((u16)rom[o + 1u] << 8);
    }
}

static int nature_power_moves_valid(const u16 *words, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
    {
        u16 m = words[i];

        if (m == 0u || (u32)m >= (u32)MOVES_COUNT)
            return 0;
    }

    return 1;
}

static int terrain_to_types_valid(const u8 *bytes, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
    {
        if ((unsigned)bytes[i] >= (unsigned)NUMBER_OF_MON_TYPES)
            return 0;
    }

    return 1;
}

void firered_portable_rom_battle_terrain_tables_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;
    size_t u16_block = (size_t)NATURE_POWER_MOVES_ROW_COUNT * 2u;
    size_t u8_off;

    s_battle_terrain_tables_rom_active = 0;

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_BATTLE_TERRAIN_TABLE_PACK_OFF",
            s_battle_terrain_tables_profile_rows, NELEMS(s_battle_terrain_tables_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (pack_off > rom_size || FIRERED_ROM_BATTLE_TERRAIN_TABLE_PACK_BYTE_SIZE > rom_size - pack_off)
        return;

    read_le_u16_block(rom, pack_off, (size_t)NATURE_POWER_MOVES_ROW_COUNT, s_nature_power_moves_rom);
    u8_off = pack_off + u16_block;
    memcpy(s_terrain_to_type_rom, rom + u8_off, (size_t)NATURE_POWER_MOVES_ROW_COUNT);

    if (!nature_power_moves_valid(s_nature_power_moves_rom, (size_t)NATURE_POWER_MOVES_ROW_COUNT))
        return;
    if (!terrain_to_types_valid(s_terrain_to_type_rom, (size_t)NATURE_POWER_MOVES_ROW_COUNT))
        return;

    s_battle_terrain_tables_rom_active = 1;
}

const u16 *firered_portable_rom_battle_terrain_nature_power_moves(void)
{
    if (!s_battle_terrain_tables_rom_active)
        return NULL;
    return s_nature_power_moves_rom;
}

const u8 *firered_portable_rom_battle_terrain_to_type(void)
{
    if (!s_battle_terrain_tables_rom_active)
        return NULL;
    return s_terrain_to_type_rom;
}

#endif /* PORTABLE */
