#include "global.h"

#include "portable/firered_portable_rom_battle_ai_fragment_prefix.h"

#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PORTABLE

const u8 *firered_portable_rom_battle_ai_fragment_prefix_ptr(u32 fragment_index)
{
    (void)fragment_index;
    return NULL;
}

#else

#include "portable/firered_portable_rom_u32_table.h"

extern const uint32_t gFireredPortableBattleAiPtrCount;

static const FireredRomU32TableProfileRow s_battle_ai_fragment_prefix_profile_rows[] = {
    { 0u, "__firered_builtin_battle_ai_fragment_prefix_profile_none__", 0u },
};

static bool8 resolve_battle_ai_fragments_rom_base(size_t *out_off)
{
    if (firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_BATTLE_AI_FRAGMENTS_TABLE_OFF",
            s_battle_ai_fragment_prefix_profile_rows, NELEMS(s_battle_ai_fragment_prefix_profile_rows), out_off))
        return TRUE;
    return firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_BATTLE_AI_FRAGMENT_PREFIX_OFF",
        s_battle_ai_fragment_prefix_profile_rows, NELEMS(s_battle_ai_fragment_prefix_profile_rows), out_off);
}

static int gba_battle_ai_fragment_in_loaded_rom(u32 addr)
{
    size_t rom_size;
    size_t off;

    if (addr < 0x08000000u || addr >= 0x0A000000u)
        return 0;

    rom_size = engine_memory_get_loaded_rom_size();
    off = (size_t)(addr - 0x08000000u);
    if (off >= rom_size)
        return 0;
    return 1;
}

const u8 *firered_portable_rom_battle_ai_fragment_prefix_ptr(u32 fragment_index)
{
    size_t n;
    size_t table_off;
    size_t rom_size;
    const u8 *rom;
    u32 addr;

    n = (size_t)gFireredPortableBattleAiPtrCount;
    if (n == 0u || (size_t)fragment_index >= n)
        return NULL;

    if (!resolve_battle_ai_fragments_rom_base(&table_off))
        return NULL;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (!firered_portable_rom_u32_table_read_entry(rom_size, rom, table_off, n, (size_t)fragment_index, &addr))
        return NULL;

    if (addr == 0u)
        return NULL;

    if (!gba_battle_ai_fragment_in_loaded_rom(addr))
        return NULL;

    return (const u8 *)(uintptr_t)addr;
}

#endif /* PORTABLE */
