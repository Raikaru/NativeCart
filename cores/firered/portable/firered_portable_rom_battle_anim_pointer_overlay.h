#ifndef GUARD_FIRERED_PORTABLE_ROM_BATTLE_ANIM_POINTER_OVERLAY_H
#define GUARD_FIRERED_PORTABLE_ROM_BATTLE_ANIM_POINTER_OVERLAY_H

#include "global.h"

/*
 * Optional ROM-backed u32[] parallel to gFireredPortableBattleAnimPtrs[] for portable tokens
 * 0x84000000 + index (firered_portable_resolve_script_ptr).
 *
 * Not the same table as gBattleAnims_General / B_ANIM_* indices used by LaunchBattleAnimation —
 * that path uses FIRERED_ROM_BATTLE_ANIMS_GENERAL_PTRS_OFF (see
 * firered_portable_rom_battle_anims_general_table.c). This overlay matches portable token order
 * from generate_portable_battle_anim_data.py (battle_anim_scripts_portable_data.c).
 *
 * The portable pointer list mixes host AnimTask_* functions, battle-anim script bytecode
 * labels, and ROM template blobs (e.g. g*SpriteTemplate). Only words in the GBA ROM window
 * (0x08000000 - 0x09FFFFFF) inside the loaded image are accepted; use ROM word 0 for slots
 * that must keep compiled host pointers.
 */

#define FIRERED_PORTABLE_BATTLE_ANIM_ROM_OVERLAY_FIRST_INDEX 0u

const u8 *firered_portable_rom_battle_anim_pointer_overlay_ptr(u32 token_index);

#endif /* GUARD_FIRERED_PORTABLE_ROM_BATTLE_ANIM_POINTER_OVERLAY_H */
