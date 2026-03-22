#include "global.h"

#include "portable/firered_portable_rom_tutor_tables.h"

#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/species.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_tutor_tables_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

#define FIRERED_TUTOR_MOVES_BYTE_SIZE ((size_t)TUTOR_MOVE_COUNT * sizeof(u16))
#define FIRERED_TUTOR_LEARNSETS_BYTE_SIZE ((size_t)NUM_SPECIES * sizeof(u16))
#define FIRERED_TUTOR_LEARNSET_MAX ((u16)((1u << TUTOR_MOVE_COUNT) - 1u))

static const FireredRomU32TableProfileRow s_tutor_moves_profile_rows[] = {
    { 0u, "__firered_builtin_tutor_moves_profile_none__", 0u },
};

static const FireredRomU32TableProfileRow s_tutor_learnsets_profile_rows[] = {
    { 0u, "__firered_builtin_tutor_learnsets_profile_none__", 0u },
};

static u16 s_tutor_moves_rom[TUTOR_MOVE_COUNT];
static u16 s_tutor_learnsets_rom[NUM_SPECIES];
static u8 s_tutor_tables_active;

const u16 *gTutorMovesActive;
const u16 *gTutorLearnsetsActive;

static void tutor_tables_sync_active_ptrs(void)
{
    if (s_tutor_tables_active)
    {
        gTutorMovesActive = s_tutor_moves_rom;
        gTutorLearnsetsActive = s_tutor_learnsets_rom;
    }
    else
    {
        gTutorMovesActive = NULL;
        gTutorLearnsetsActive = NULL;
    }
}

static int tutor_moves_rom_valid(const u16 *moves)
{
    size_t i;

    for (i = 0; i < TUTOR_MOVE_COUNT; i++)
    {
        if (moves[i] >= MOVES_COUNT)
            return 0;
    }
    return 1;
}

static int tutor_learnsets_rom_valid(const u16 *ls)
{
    size_t i;

    if (ls[SPECIES_NONE] != 0)
        return 0;

    for (i = 0; i < NUM_SPECIES; i++)
    {
        if (ls[i] > FIRERED_TUTOR_LEARNSET_MAX)
            return 0;
    }
    return 1;
}

void firered_portable_rom_tutor_tables_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t moves_off, ls_off;

    s_tutor_tables_active = 0;
    tutor_tables_sync_active_ptrs();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_TUTOR_MOVES_TABLE_OFF",
            s_tutor_moves_profile_rows, NELEMS(s_tutor_moves_profile_rows), &moves_off))
        return;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_TUTOR_LEARNSETS_TABLE_OFF",
            s_tutor_learnsets_profile_rows, NELEMS(s_tutor_learnsets_profile_rows), &ls_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (moves_off > rom_size || FIRERED_TUTOR_MOVES_BYTE_SIZE > rom_size - moves_off)
        return;
    if (ls_off > rom_size || FIRERED_TUTOR_LEARNSETS_BYTE_SIZE > rom_size - ls_off)
        return;

    memcpy(s_tutor_moves_rom, rom + moves_off, FIRERED_TUTOR_MOVES_BYTE_SIZE);
    memcpy(s_tutor_learnsets_rom, rom + ls_off, FIRERED_TUTOR_LEARNSETS_BYTE_SIZE);

    if (!tutor_moves_rom_valid(s_tutor_moves_rom))
        return;
    if (!tutor_learnsets_rom_valid(s_tutor_learnsets_rom))
        return;

    s_tutor_tables_active = 1;
    tutor_tables_sync_active_ptrs();
}

#endif /* PORTABLE */
