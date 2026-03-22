#include "global.h"

#include "portable/firered_portable_rom_std_scripts_table.h"

#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PORTABLE

const u8 *firered_portable_rom_std_script_ptr(u8 std_idx)
{
    (void)std_idx;
    return NULL;
}

#else

#include "portable/firered_portable_rom_u32_table.h"

extern const u8 *const gStdScripts[];
extern const u8 *const gStdScriptsEnd[];

/* Sentinel row keeps the array non-empty; profile_id never matches real compat ids. */
static const FireredRomU32TableProfileRow s_std_scripts_profile_rows[] = {
    { 0u, "__firered_builtin_std_scripts_profile_none__", 0u },
};

static size_t std_scripts_entry_count(void)
{
    return (size_t)(gStdScriptsEnd - gStdScripts);
}

static int gba_script_ptr_in_loaded_rom(u32 addr)
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

const u8 *firered_portable_rom_std_script_ptr(u8 std_idx)
{
    size_t n;
    size_t table_off;
    size_t rom_size;
    const u8 *rom;
    u32 addr;

    n = std_scripts_entry_count();
    if (n == 0u || (size_t)std_idx >= n)
        return NULL;

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_STD_SCRIPTS_TABLE_OFF",
            s_std_scripts_profile_rows, NELEMS(s_std_scripts_profile_rows), &table_off))
        return NULL;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (!firered_portable_rom_u32_table_read_entry(rom_size, rom, table_off, n, (size_t)std_idx, &addr))
        return NULL;

    if (addr == 0u)
        return NULL;

    if (!gba_script_ptr_in_loaded_rom(addr))
        return NULL;

    return (const u8 *)(uintptr_t)addr;
}

#endif /* PORTABLE */
