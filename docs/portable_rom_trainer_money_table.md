# Portable ROM: trainer money multipliers (`gTrainerMoneyTable`)

## Purpose

Post-battle trainer money (`battle_script_commands.c`, `Cmd_*` money reward path) uses a **null-terminated** table of **`struct TrainerMoney`** (**`u8 classId`**, **`u8 value`**). Portable builds can replace it from ROM when **`FIRERED_ROM_TRAINER_MONEY_TABLE_OFF`** (or a profile row) validates.

## Compiled fallback

**`gTrainerMoneyTable_Compiled[]`** in **`battle_main.c`**; **`gTrainerMoneyTable`** is a **`PORTABLE`** macro selecting **`gTrainerMoneyTableActive`** (ROM copy) or compiled.

## Wire format

- Contiguous **`struct TrainerMoney`** rows (host layout, **2** bytes each), **little-endian** field order matches struct.
- **Terminator:** exactly one row with **`classId == 0xFF`**. **`value`** must be **non-zero** (vanilla uses **`5`** as default multiplier when class is not listed).
- **Non-terminator rows:** **`classId != 0xFF`**, **`value != 0`**.
- **Max rows:** **256** including terminator (loader copies up to that many pairs, finds terminator, validates, zero-pads after).

## Activation

- **`FIRERED_ROM_TRAINER_MONEY_TABLE_OFF`** — hex file offset to the first **`TrainerMoney`** row.
- Profile: **`s_trainer_money_profile_rows`** in **`firered_portable_rom_trainer_money_table.c`**.

All-or-nothing: invalid layout → inactive → compiled table via macro.

## Related

- **`cores/firered/portable/firered_portable_rom_trainer_money_table.{h,c}`**
- **`include/battle_main.h`** (`gTrainerMoneyTable` macro)
- Offline: **`tools/portable_generators/validate_trainer_money_rom_pack.py`** (see `tools/docs/README_validators.md`)
