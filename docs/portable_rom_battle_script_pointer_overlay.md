# ROM-backed battle script pointer overlay (`0x80000000` resolver)

## Purpose

Portable tokens **`0x80000000 + i`** resolve through **`gFireredPortableBattleScriptPtrs[i]`** (`battle_scripts_portable_data.c`). Hacks that **relocate** battle script bytecode in ROM while keeping the **same portable token order** can supply a parallel **`u32[]`** of GBA ROM pointers.

## Interleaved table (no index prefix skip)

Unlike **`0x81000000`** (ScrCmd prefix), the battle-script pointer list mixes:

- **`BattleScript_*`** (bytecode in ROM),
- **`&g*`** (EWRAM/IWRAM fields),
- **`s*` / `c*`** and related **scripting/communication** symbols.

There is **no** fixed **first index** where “everything is ROM bytecode.”

**Safety rule:** the overlay **only returns** a pointer when the ROM word is **non-zero**, lies in **`0x08000000`–`0x09FFFFFF`**, and is **inside the loaded ROM image**. Words that are **EWRAM/IWRAM** (or anything outside that window) **fail validation** → resolver uses **`gFireredPortableBattleScriptPtrs[i]`** unchanged.

For a hand-built parallel table, use **`0`** for slots that should always use compiled pointers, or store only **ROM** addresses for relocated **`BattleScript_*`** entries.

## Configuration

1. **`FIRERED_ROM_BATTLE_SCRIPT_PTRS_OFF`** — hex file offset to **`gFireredPortableBattleScriptPtrCount`** little-endian `u32` values, **index-aligned** with `gFireredPortableBattleScriptPtrs` (`generate_portable_battle_script_data.py` **token_order**).
2. **Profile rows** (optional): `s_battle_script_ptr_overlay_profile_rows` in `firered_portable_rom_battle_script_pointer_overlay.c`.

## Not in scope

- **`FIRERED_ROM_BATTLE_AI_SCRIPTS_TABLE_OFF`** — top-level **AI** script table (`BattleAI_DoAIProcessing`), not this resolver family.
- **`0x83000000`** fragments — separate overlay (`firered_portable_rom_battle_ai_fragment_prefix`).

## Related

- `cores/firered/portable/firered_portable_rom_battle_script_pointer_overlay.{h,c}`
- `cores/firered/portable/portable_script_pointer_resolver.c`
- `docs/rom_backed_runtime.md` §6j
