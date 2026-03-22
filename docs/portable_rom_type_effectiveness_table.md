# ROM-backed type chart (`gTypeEffectiveness`, PORTABLE)

## Purpose

**`gTypeEffectiveness`** is the Gen 3 **type effectiveness** list: packed **triplets** `(attacking type, defending type, damage multiplier)` scanned by battle code until **`TYPE_ENDTABLE`**. Hacks that **rebalance typings** (e.g. Fairy, chart edits) need the loaded ROM’s bytes, not only the decomp snapshot.

## Layout (contract)

Matches **`src_transformed/battle_main.c`** / vanilla FireRed:

- **Size:** exactly **`FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT`** (**336**) bytes.
- **Record:** repeating **3 × `u8`**: atk type, def type, multiplier.
- **Multipliers:** only **`TYPE_MUL_*`** values used in vanilla: **0, 5, 10, 20** (`TYPE_MUL_NO_EFFECT` … `TYPE_MUL_SUPER_EFFECTIVE`).
- **`TYPE_FORESIGHT` (0xFE):** sentinel row uses **atk == def == `TYPE_FORESIGHT`** (vanilla layout).
- **Terminator:** last triplet is **`TYPE_ENDTABLE`, `TYPE_ENDTABLE`,** and a valid multiplier (vanilla uses **`TYPE_MUL_NO_EFFECT`**).
- **Types:** for normal rows (not foresight), atk/def must be **`< NUMBER_OF_MON_TYPES`** (18 on FireRed).

Raw **file offset** into the mapped ROM image (same as other `FIRERED_ROM_*_OFF` env vars).

## Configuration

- **`FIRERED_ROM_TYPE_EFFECTIVENESS_OFF`** — hex offset (via `firered_portable_rom_u32_table_resolve_base_off`).
- **Profile rows** in `firered_portable_rom_type_effectiveness_table.c`.

## Behaviour

After ROM load, **`firered_portable_rom_type_effectiveness_table_refresh_after_rom_load()`** copies **336** bytes and validates structure. On success, **`gTypeEffectivenessActivePtr`** points at the cache; otherwise it points at compiled **`gTypeEffectiveness`**.

**`TYPE_EFFECT_ATK_TYPE` / `TYPE_EFFECT_DEF_TYPE` / `TYPE_EFFECT_MULTIPLIER`** (in `battle_main.h`) read through that pointer on **PORTABLE** only. Non-portable builds keep direct **`gTypeEffectiveness`** indexing.

## Limitations

- **Fixed 336 bytes** and **18 mon types** in validation — hacks that **lengthen** the table or add types **without** matching this contract must **rebuild** the engine or leave the env unset (compiled fallback).

## Related

- `cores/firered/portable/firered_portable_rom_type_effectiveness_table.{h,c}`
- `include/battle_main.h`
