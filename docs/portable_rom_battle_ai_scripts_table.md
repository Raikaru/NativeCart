# ROM-backed `gBattleAI_ScriptsTable` (PORTABLE / SDL)

## Purpose

Top-level battle AI dispatch uses **`gBattleAI_ScriptsTable[aiLogicId]`** (30 entries: `AI_CheckBadMove` … `AI_FirstBattle`, including `AI_Ret` slots). Portable mirrors this in `battle_ai_scripts_portable_data.c`.

Embedded pointers **inside** those scripts still use **`0x83000000 + i`** tokens resolved by **`firered_portable_resolve_script_ptr`** → **`gFireredPortableBattleAiPtrs`** — **unchanged** by this seam.

This ROM table only overrides **where each primary AI script starts** when the loaded ROM’s pointer block is known.

## Configuration

1. **`FIRERED_ROM_BATTLE_AI_SCRIPTS_TABLE_OFF`** — hex file offset to **30** little-endian `u32` GBA ROM addresses, same order as `gBattleAI_ScriptsTable[]` in `pokefirered_core/generated/src/data/battle_ai_scripts_portable_data.c`.
2. **`BATTLE_AI_SCRIPTS_TABLE_COUNT`** in `include/constants/battle_ai.h` must stay in sync if the generated table length ever changes.
3. Optional profile rows: `s_battle_ai_scripts_profile_rows` in `firered_portable_rom_battle_ai_scripts_table.c`.

## Runtime

- **`BattleAI_DoAIProcessing`** (`AIState_SettingUp`, PORTABLE): try ROM pointer for `aiLogicId`; if NULL or invalid, use **`gBattleAI_ScriptsTable[aiLogicId]`** (vanilla).

## Related

- `firered_portable_rom_u32_table.{h,c}`
- `portable_script_pointer_resolver.c` — token range `0x83000000` for **fragment** pointers only
- `docs/rom_backed_runtime.md` §6f
