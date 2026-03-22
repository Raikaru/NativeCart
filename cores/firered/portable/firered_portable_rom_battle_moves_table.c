#include "global.h"

#include "portable/firered_portable_rom_battle_moves_table.h"

#include "constants/battle_move_effects.h"
#include "constants/moves.h"
#include "constants/pokemon.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_battle_moves_table_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

#define FIRERED_BATTLE_MOVE_WIRE_BYTES ((size_t)sizeof(struct BattleMove))
#define FIRERED_BATTLE_MOVE_ROM_ENTRY_COUNT ((size_t)MOVES_COUNT)
#define FIRERED_BATTLE_MOVES_TABLE_BYTE_SIZE (FIRERED_BATTLE_MOVE_ROM_ENTRY_COUNT * FIRERED_BATTLE_MOVE_WIRE_BYTES)

static const FireredRomU32TableProfileRow s_battle_moves_profile_rows[] = {
    { 0u, "__firered_builtin_battle_moves_profile_none__", 0u },
};

static struct BattleMove s_battle_moves_rom[MOVES_COUNT];
static u8 s_battle_moves_rom_active;

/* NULL => use compiled gBattleMoves_Compiled (see pokemon.h macro). */
const struct BattleMove *gBattleMovesActive;

_Static_assert(FIRERED_BATTLE_MOVE_WIRE_BYTES == 9u, "BattleMove wire must be 9 bytes");
_Static_assert(FIRERED_BATTLE_MOVES_TABLE_BYTE_SIZE == sizeof(s_battle_moves_rom), "battle moves cache size");

static void battle_moves_sync_active_ptr(void)
{
    gBattleMovesActive = s_battle_moves_rom_active ? (const struct BattleMove *)s_battle_moves_rom : NULL;
}

static int battle_moves_rom_valid(const struct BattleMove *tbl)
{
    size_t i;

    for (i = 0; i < MOVES_COUNT; i++)
    {
        const struct BattleMove *m = &tbl[i];

        if (m->type >= NUMBER_OF_MON_TYPES)
            return 0;
        /* 0 = never-miss / special handling in UI; otherwise standard accuracy. */
        if (m->accuracy > 100)
            return 0;
        if (m->pp > 80)
            return 0;
        if (m->secondaryEffectChance > 100)
            return 0;
        /* Target is a bitmask of MOVE_TARGET_* (see include/battle.h). */
        if (m->target > 0x7Fu)
            return 0;
        if (m->effect > EFFECT_CAMOUFLAGE)
            return 0;
    }

    /* MOVE_NONE row: vanilla "empty" move contract (safe default for index 0). */
    if (tbl[MOVE_NONE].effect != EFFECT_HIT
     || tbl[MOVE_NONE].power != 0
     || tbl[MOVE_NONE].type != TYPE_NORMAL
     || tbl[MOVE_NONE].accuracy != 0
     || tbl[MOVE_NONE].pp != 0
     || tbl[MOVE_NONE].secondaryEffectChance != 0
     || tbl[MOVE_NONE].target != 0 /* MOVE_TARGET_SELECTED */
     || tbl[MOVE_NONE].priority != 0
     || tbl[MOVE_NONE].flags != 0)
        return 0;

    return 1;
}

void firered_portable_rom_battle_moves_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;

    s_battle_moves_rom_active = 0;
    battle_moves_sync_active_ptr();

    need = FIRERED_BATTLE_MOVES_TABLE_BYTE_SIZE;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_BATTLE_MOVES_TABLE_OFF",
            s_battle_moves_profile_rows, NELEMS(s_battle_moves_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    memcpy(s_battle_moves_rom, rom + table_off, need);
    if (!battle_moves_rom_valid(s_battle_moves_rom))
        return;

    s_battle_moves_rom_active = 1;
    battle_moves_sync_active_ptr();
}

#endif /* PORTABLE */
