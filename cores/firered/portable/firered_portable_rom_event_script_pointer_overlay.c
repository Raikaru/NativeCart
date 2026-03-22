#include "global.h"

#include "portable/firered_portable_rom_event_script_pointer_overlay.h"

#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PORTABLE

const u8 *firered_portable_rom_event_script_pointer_overlay_ptr(u32 token_index)
{
    (void)token_index;
    return NULL;
}

#else

#include "portable/firered_portable_rom_u32_table.h"

extern const uint32_t gFireredPortableEventScriptPtrCount;

static const FireredRomU32TableProfileRow s_event_script_ptr_overlay_profile_rows[] = {
    { 0u, "__firered_builtin_event_script_ptr_overlay_profile_none__", 0u },
};

static int gba_event_script_ptr_in_loaded_rom(u32 addr)
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

const u8 *firered_portable_rom_event_script_pointer_overlay_ptr(u32 token_index)
{
    size_t n;
    size_t table_off;
    size_t rom_size;
    const u8 *rom;
    u32 addr;

    if (token_index < FIRERED_PORTABLE_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX)
        return NULL;

    n = (size_t)gFireredPortableEventScriptPtrCount;
    if (n == 0u || (size_t)token_index >= n)
        return NULL;

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_EVENT_SCRIPT_PTRS_OFF",
            s_event_script_ptr_overlay_profile_rows, NELEMS(s_event_script_ptr_overlay_profile_rows), &table_off))
        return NULL;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (!firered_portable_rom_u32_table_read_entry(rom_size, rom, table_off, n, (size_t)token_index, &addr))
        return NULL;

    if (addr == 0u)
        return NULL;

    if (!gba_event_script_ptr_in_loaded_rom(addr))
        return NULL;

    return (const u8 *)(uintptr_t)addr;
}

#endif /* PORTABLE */
