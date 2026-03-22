#include "global.h"

#include "portable/firered_portable_rom_special_vars_table.h"

#include "constants/vars.h"

#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PORTABLE

u16 *firered_portable_rom_special_var_ptr(u8 slot_index)
{
    (void)slot_index;
    return NULL;
}

#else

#include "portable/firered_portable_rom_u32_table.h"

enum { SPECIAL_VAR_SLOT_COUNT = (int)(SPECIAL_VARS_END - SPECIAL_VARS_START + 1) };

static const FireredRomU32TableProfileRow s_special_vars_profile_rows[] = {
    { 0u, "__firered_builtin_special_vars_profile_none__", 0u },
};

static int gba_ewram_u16_ptr_valid(u32 addr)
{
    uintptr_t p = (uintptr_t)addr;

    if ((p & 1u) != 0)
        return 0;
    if (p < (uintptr_t)ENGINE_EWRAM_ADDR || p >= (uintptr_t)ENGINE_EWRAM_ADDR + ENGINE_EWRAM_SIZE)
        return 0;
    return 1;
}

u16 *firered_portable_rom_special_var_ptr(u8 slot_index)
{
    size_t table_off;
    size_t rom_size;
    const u8 *rom;
    u32 addr;
    size_t n;

    n = (size_t)SPECIAL_VAR_SLOT_COUNT;
    if (n == 0u || (size_t)slot_index >= n)
        return NULL;

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_SPECIAL_VARS_TABLE_OFF",
            s_special_vars_profile_rows, NELEMS(s_special_vars_profile_rows), &table_off))
        return NULL;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (!firered_portable_rom_u32_table_read_entry(rom_size, rom, table_off, n, (size_t)slot_index, &addr))
        return NULL;

    if (addr == 0u)
        return NULL;

    if (!gba_ewram_u16_ptr_valid(addr))
        return NULL;

    return (u16 *)(uintptr_t)addr;
}

#endif /* PORTABLE */
