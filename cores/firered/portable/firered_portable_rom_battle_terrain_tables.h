#ifndef GUARD_FIRERED_PORTABLE_ROM_BATTLE_TERRAIN_TABLES_H
#define GUARD_FIRERED_PORTABLE_ROM_BATTLE_TERRAIN_TABLES_H

#include "global.h"

#include "constants/nature_power_moves.h"

#include <stddef.h>

/*
 * Single packed blob for **outdoor** battle terrain helpers (indices **0..9** =
 * BATTLE_TERRAIN_GRASS .. BATTLE_TERRAIN_PLAIN):
 *
 * 1) **10 × little-endian u16** — Nature Power move ids (same order as
 *    sNaturePowerMoves_Compiled[]).
 * 2) **10 × u8** — types for Camouflage-style terrain → type (same order as
 *    sTerrainToType_Compiled[]).
 *
 * **Total:** **`NATURE_POWER_MOVES_ROW_COUNT * 3`** bytes (**30** for retail).
 *
 * Env: **FIRERED_ROM_BATTLE_TERRAIN_TABLE_PACK_OFF** (hex file offset).
 * Profile: **FireredRomU32TableProfileRow** in **firered_portable_rom_battle_terrain_tables.c**.
 *
 * Validation: each move **0 < id < MOVES_COUNT**; each type **< NUMBER_OF_MON_TYPES**.
 * **All-or-nothing** — either both slices activate or both fall back to compiled.
 */

#define FIRERED_ROM_BATTLE_TERRAIN_TABLE_PACK_BYTE_SIZE ((size_t)NATURE_POWER_MOVES_ROW_COUNT * 3u)

void firered_portable_rom_battle_terrain_tables_refresh_after_rom_load(void);

/* NULL => use compiled tables in battle_script_commands.c */
const u16 *firered_portable_rom_battle_terrain_nature_power_moves(void);
const u8 *firered_portable_rom_battle_terrain_to_type(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_BATTLE_TERRAIN_TABLES_H */
