#ifndef GUARD_FIRERED_PORTABLE_ROM_BATTLE_AI_FRAGMENT_PREFIX_H
#define GUARD_FIRERED_PORTABLE_ROM_BATTLE_AI_FRAGMENT_PREFIX_H

#include "global.h"

/*
 * ROM-backed parallel table for **gFireredPortableBattleAiPtrs** (token 0x83000000 + index).
 * Same entry count as gFireredPortableBattleAiPtrCount; see docs/portable_rom_battle_ai_fragment_prefix.md.
 */

const u8 *firered_portable_rom_battle_ai_fragment_prefix_ptr(u32 fragment_index);

#endif /* GUARD_FIRERED_PORTABLE_ROM_BATTLE_AI_FRAGMENT_PREFIX_H */
