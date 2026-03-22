#ifndef GUARD_FIRERED_PORTABLE_ROM_BATTLE_SCRIPT_POINTER_OVERLAY_H
#define GUARD_FIRERED_PORTABLE_ROM_BATTLE_SCRIPT_POINTER_OVERLAY_H

#include "global.h"

/*
 * Optional ROM-backed u32[] parallel to gFireredPortableBattleScriptPtrs[] for portable
 * script pointer tokens 0x80000000 + index.
 *
 * The battle-script token table is interleaved: BattleScript_* bytecode, &g* RAM pointers,
 * s-prefixed and c-prefixed scripting or communication symbols, etc. No single index prefix
 * is "safe" vs "unsafe" for ROM.
 *
 * The overlay only accepts pointers in the GBA ROM window (0x08000000 - 0x09FFFFFF) within
 * the loaded cartridge. EWRAM or IWRAM (e.g. gCurrentMove) and other non-ROM addresses fail
 * validation; the resolver then uses gFireredPortableBattleScriptPtrs[i] (vanilla-safe).
 *
 * FIRERED_PORTABLE_BATTLE_SCRIPT_ROM_OVERLAY_FIRST_INDEX is 0. Use word 0 in ROM for slots
 * that should always use compiled pointers, or rely on validation failure.
 * See docs/portable_rom_battle_script_pointer_overlay.md.
 */

#define FIRERED_PORTABLE_BATTLE_SCRIPT_ROM_OVERLAY_FIRST_INDEX 0u

const u8 *firered_portable_rom_battle_script_pointer_overlay_ptr(u32 token_index);

#endif /* GUARD_FIRERED_PORTABLE_ROM_BATTLE_SCRIPT_POINTER_OVERLAY_H */
