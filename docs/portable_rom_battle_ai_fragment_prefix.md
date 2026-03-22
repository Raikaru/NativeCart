# ROM-backed battle-AI fragment table (`0x83000000` resolver)

## Purpose

Portable battle-AI bytecode embeds **`0x83000000 + index`** tokens. **`firered_portable_resolve_script_ptr`** maps them to **`gFireredPortableBattleAiPtrs[index]`**, then **`firered_portable_fix_battle_ai_ptr`**.

This seam adds an optional ROM-backed **`u32[]` parallel to the full compiled table**: **`gFireredPortableBattleAiPtrCount`** entries, same order as **`gFireredPortableBattleAiPtrs[]`**. Any index in range can resolve from ROM when the table is configured and valid; otherwise the compiled pointer is used.

## Configuration

1. **Env (either name — same semantics):**
   - **`FIRERED_ROM_BATTLE_AI_FRAGMENTS_TABLE_OFF`** (preferred), or  
   - **`FIRERED_ROM_BATTLE_AI_FRAGMENT_PREFIX_OFF`** (legacy alias)
   — hex file offset to **`gFireredPortableBattleAiPtrCount`** consecutive little-endian `u32` GBA ROM addresses, matching **`gFireredPortableBattleAiPtrs[0 .. count-1]`** in `battle_ai_scripts_portable_data.c`.

2. The **entire** table must lie inside the loaded ROM image (bounds check is for the full `4 × count` bytes). A **partial** dump (e.g. old 128-word setup) fails bounds → ROM path returns NULL → **compiled fallback** for all indices (safe).

3. Optional profile rows: `s_battle_ai_fragment_prefix_profile_rows` in `firered_portable_rom_battle_ai_fragment_prefix.c`.

## Runtime

- **`firered_portable_resolve_script_ptr`**: ROM try via **`firered_portable_rom_battle_ai_fragment_prefix_ptr(i)`**; if NULL, **`gFireredPortableBattleAiPtrs[i]`**; then **`firered_portable_fix_battle_ai_ptr`**.
- **`gBattleAI_ScriptsTable`** ROM override remains separate (`FIRERED_ROM_BATTLE_AI_SCRIPTS_TABLE_OFF`).

## Related

- `firered_portable_rom_u32_table.{h,c}`
- `docs/rom_backed_runtime.md` §6g
