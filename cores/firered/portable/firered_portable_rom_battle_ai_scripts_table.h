#ifndef GUARD_FIRERED_PORTABLE_ROM_BATTLE_AI_SCRIPTS_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_BATTLE_AI_SCRIPTS_TABLE_H

#include "global.h"

/*
 * Optional ROM-backed table parallel to **gBattleAI_ScriptsTable** (top-level AI scripts by aiLogicId).
 * Adjacent to portable **0x83000000** battle-AI token resolution; does not replace gFireredPortableBattleAiPtrs.
 */

const u8 *firered_portable_rom_battle_ai_script_ptr(u8 ai_logic_id);

#endif /* GUARD_FIRERED_PORTABLE_ROM_BATTLE_AI_SCRIPTS_TABLE_H */
