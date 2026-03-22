#include "global.h"

#include "portable/firered_portable_rom_field_effect_script_pointers.h"

#include "constants/field_effects.h"

#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PORTABLE

const u8 *firered_portable_rom_field_effect_script_ptr(u8 fldeff)
{
    (void)fldeff;
    return NULL;
}

#else

#include "portable/firered_portable_rom_u32_table.h"

static const FireredRomU32TableProfileRow s_field_effect_script_ptrs_profile_rows[] = {
    { 0u, "__firered_builtin_field_effect_script_ptrs_profile_none__", 0u },
};

static size_t field_effect_script_entry_count(void)
{
    return (size_t)(FLDEFF_PHOTO_FLASH + 1);
}

static int gba_field_effect_script_start_in_loaded_rom(u32 addr)
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

const u8 *firered_portable_rom_field_effect_script_ptr(u8 fldeff)
{
    size_t n;
    size_t table_off;
    size_t rom_size;
    const u8 *rom;
    u32 addr;

    n = field_effect_script_entry_count();
    if (n == 0u || (size_t)fldeff >= n)
        return NULL;

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_FIELD_EFFECT_SCRIPT_PTRS_OFF",
            s_field_effect_script_ptrs_profile_rows, NELEMS(s_field_effect_script_ptrs_profile_rows), &table_off))
        return NULL;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (!firered_portable_rom_u32_table_read_entry(rom_size, rom, table_off, n, (size_t)fldeff, &addr))
        return NULL;

    if (addr == 0u)
        return NULL;

    if (!gba_field_effect_script_start_in_loaded_rom(addr))
        return NULL;

    return (const u8 *)(uintptr_t)addr;
}

#endif /* PORTABLE */
