#include "global.h"

#include "portable/firered_portable_rom_battle_anims_general_table.h"

#include "constants/battle_anim.h"

#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PORTABLE

const u8 *firered_portable_rom_battle_anim_general_script_ptr(u16 table_id)
{
    (void)table_id;
    return NULL;
}

#else

#include "portable/firered_portable_rom_u32_table.h"

static const FireredRomU32TableProfileRow s_battle_anims_general_profile_rows[] = {
    { 0u, "__firered_builtin_battle_anims_general_profile_none__", 0u },
};

static size_t battle_anims_general_entry_count(void)
{
    return (size_t)(B_ANIM_SAFARI_REACTION + 1);
}

static int gba_battle_anim_script_start_in_loaded_rom(u32 addr)
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

const u8 *firered_portable_rom_battle_anim_general_script_ptr(u16 table_id)
{
    size_t n;
    size_t table_off;
    size_t rom_size;
    const u8 *rom;
    u32 addr;

    n = battle_anims_general_entry_count();
    if (n == 0u || (u32)table_id >= n)
        return NULL;

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_BATTLE_ANIMS_GENERAL_PTRS_OFF",
            s_battle_anims_general_profile_rows, NELEMS(s_battle_anims_general_profile_rows), &table_off))
        return NULL;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (!firered_portable_rom_u32_table_read_entry(rom_size, rom, table_off, n, (size_t)table_id, &addr))
        return NULL;

    if (addr == 0u)
        return NULL;

    if (!gba_battle_anim_script_start_in_loaded_rom(addr))
        return NULL;

    return (const u8 *)(uintptr_t)addr;
}

#endif /* PORTABLE */
