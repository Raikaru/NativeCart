#include "global.h"

#include "portable/firered_portable_rom_tmhm_learnsets_table.h"

#include "constants/species.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_tmhm_learnsets_table_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

#define FIRERED_TMHM_LEARNSET_ROM_ENTRY_COUNT ((size_t)NUM_SPECIES)
#define FIRERED_TMHM_LEARNSET_ROW_U32S ((size_t)2)
#define FIRERED_TMHM_LEARNSET_TABLE_BYTE_SIZE (FIRERED_TMHM_LEARNSET_ROM_ENTRY_COUNT * FIRERED_TMHM_LEARNSET_ROW_U32S * sizeof(u32))

static const FireredRomU32TableProfileRow s_tmhm_learnsets_profile_rows[] = {
    { 0u, "__firered_builtin_tmhm_learnsets_profile_none__", 0u },
};

static u32 s_tmhm_learnsets_rom[FIRERED_TMHM_LEARNSET_ROM_ENTRY_COUNT][FIRERED_TMHM_LEARNSET_ROW_U32S];
static u8 s_tmhm_learnsets_rom_active;

/* NULL => use compiled sTMHMLearnsets_Compiled (see pokemon.c macro). */
const u32 (*gTMHMLearnsetsActive)[2];

static void tmhm_learnsets_sync_active_ptr(void)
{
    gTMHMLearnsetsActive = s_tmhm_learnsets_rom_active ? s_tmhm_learnsets_rom : NULL;
}

static int tmhm_learnsets_rom_valid(const u32 (*buf)[2])
{
    /* SPECIES_NONE must have empty learnset (vanilla contract). */
    if (buf[SPECIES_NONE][0] != 0u || buf[SPECIES_NONE][1] != 0u)
        return 0;
    return 1;
}

void firered_portable_rom_tmhm_learnsets_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;

    s_tmhm_learnsets_rom_active = 0;
    tmhm_learnsets_sync_active_ptr();

    need = FIRERED_TMHM_LEARNSET_TABLE_BYTE_SIZE;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_TMHM_LEARNSETS_TABLE_OFF",
            s_tmhm_learnsets_profile_rows, NELEMS(s_tmhm_learnsets_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    memcpy(s_tmhm_learnsets_rom, rom + table_off, need);
    if (!tmhm_learnsets_rom_valid(s_tmhm_learnsets_rom))
        return;

    s_tmhm_learnsets_rom_active = 1;
    tmhm_learnsets_sync_active_ptr();
}

#endif /* PORTABLE */
