#ifndef GUARD_FIRERED_PORTABLE_ROM_BATTLE_ANIMS_GENERAL_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_BATTLE_ANIMS_GENERAL_TABLE_H

#include "global.h"

/*
 * Optional ROM-backed table parallel to **gBattleAnims_General** (28 LE u32s, B_ANIM_* order).
 * See docs/portable_rom_battle_anims_general_table.md.
 */

const u8 *firered_portable_rom_battle_anim_general_script_ptr(u16 table_id);

#endif /* GUARD_FIRERED_PORTABLE_ROM_BATTLE_ANIMS_GENERAL_TABLE_H */
