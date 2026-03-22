# ROM-backed battle moves (`gBattleMoves`, PORTABLE)

## Purpose

**`gBattleMoves[]`** holds **`struct BattleMove`** metadata for every move ID (`MOVES_COUNT` entries, index **`MOVE_NONE` … `MOVES_COUNT - 1`**). Portable SDL can mirror the **loaded ROM** when a contiguous image matches this build’s layout, so hacks that **retune power, type, PP, accuracy, effects, or flags** are reflected without recompiling.

## Layout contract (wire format)

- **Rows:** **`MOVES_COUNT`** (355 on vanilla FireRed), index **`0 .. MOVES_COUNT - 1`** aligned with **`MOVE_*`**.
- **Row size:** **`9`** bytes (`FIRERED_BATTLE_MOVE_WIRE_BYTES` = **`sizeof(struct BattleMove)`** on the portable GCC build), **field order** (same as `struct BattleMove` in `include/pokemon.h`):

| Offset | Size | Field |
|--------|------|--------|
| 0 | `u8` | `effect` |
| 1 | `u8` | `power` |
| 2 | `u8` | `type` |
| 3 | `u8` | `accuracy` |
| 4 | `u8` | `pp` |
| 5 | `u8` | `secondaryEffectChance` |
| 6 | `u8` | `target` |
| 7 | `s8` | `priority` |
| 8 | `u8` | `flags` |

- **Total blob size:** **`MOVES_COUNT * 9`** bytes (e.g. **3195** for `MOVES_COUNT == 355`).

## Validation (fail-safe)

1. **`sizeof(struct BattleMove) == 9`** on the portable target (`_Static_assert` in the ROM module + header).
2. **Per-move (all indices):**
   - **`type < NUMBER_OF_MON_TYPES`**
   - **`accuracy <= 100`** ( **`0`** allowed — never-miss / UI special cases)
   - **`pp <= 80`**
   - **`secondaryEffectChance <= 100`**
   - **`target <= 0x7F`** (bitmask of **`MOVE_TARGET_*`** in `include/battle.h`)
   - **`effect <= EFFECT_CAMOUFLAGE`** (213 — last defined FireRed move effect in `include/constants/battle_move_effects.h`)
3. **`MOVE_NONE` row** must match the vanilla **empty-move** pattern: **`EFFECT_HIT`**, **`TYPE_NORMAL`**, **`MOVE_TARGET_SELECTED`**, zeros for power/accuracy/pp/secondary/priority/flags.

On any failure → ROM cache **not** activated; **`gBattleMovesActive`** stays **NULL** and the **`gBattleMoves`** macro uses **`gBattleMoves_Compiled`**.

**Hack note:** New **effect IDs** above **`EFFECT_CAMOUFLAGE`** require matching **engine** `battle_script_commands` / effect tables and **raising** the validator cap in lockstep.

## Runtime wiring

- **Env / profile base:** **`FIRERED_ROM_BATTLE_MOVES_TABLE_OFF`** (hex file offset), with the usual **`firered_portable_rom_u32_table_resolve_base_off`** profile row pattern (placeholder **`__firered_builtin_battle_moves_profile_none__`**).
- **Refresh:** **`firered_portable_rom_battle_moves_table_refresh_after_rom_load()`** from **`engine_runtime_backend.c`** (after other structural ROM tables).
- **Access:** **`include/pokemon.h`** — **`gBattleMovesActive`** + **`#define gBattleMoves`**; compiled rodata is **`gBattleMoves_Compiled[]`** in **`src/data/battle_moves.h`**. **`src/rom_header_gf.c`** uses **`gBattleMoves_Compiled`** for **`.moves`** on **PORTABLE** (static initializer must be a constant symbol, not the runtime macro).

## GBA / ROM layout caution

Retail FireRed **`struct BattleMove`** is also **9 bytes** (nine byte-sized fields including **`s8 priority`**). If a fork changes the struct (padding, wider fields), treat the wire format as **that fork’s explicit layout** and adjust **`sizeof` / validation** — do not assume **memcpy** interchangeability across mismatched ABIs.

## Non-portable builds

Symbol **`gBattleMoves`** only; no ROM refresh.
