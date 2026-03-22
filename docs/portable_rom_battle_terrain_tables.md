# ROM-backed battle terrain tables (Nature Power + terrain type, PORTABLE)

## Purpose

**`battle_script_commands.c`** uses two small **terrain-indexed** tables for outdoor
terrains **`BATTLE_TERRAIN_GRASS` (0) … `BATTLE_TERRAIN_PLAIN` (9)**:

1. **Nature Power** — move id per terrain (`Cmd_callterrainattack`).
2. **Terrain → type** — type id for **Camouflage** / **`settypetoterrain`**
   (`Cmd_settypetoterrain`).

This slice is **pointer-free**, **30 bytes** total, and uses the same **env + profile**
pattern as **`firered_portable_rom_u32_table`**.

## Layout (single pack)

**`FIRERED_ROM_BATTLE_TERRAIN_TABLE_PACK_OFF`** points to **30** bytes:

| Offset | Size | Content |
|--------|------|---------|
| 0 | 20 | **10 × LE `u16`** — Nature Power move ids |
| 20 | 10 | **10 × `u8`** — types (`TYPE_*`) |

Row **`i`** is terrain **`i`** (grass, long grass, … plain).

## Validation (fail-safe, all-or-nothing)

- Each move id: **`0 < id < MOVES_COUNT`**.
- Each type: **`< NUMBER_OF_MON_TYPES`**.

If anything fails, **both** ROM paths stay inactive and compiled
**`sNaturePowerMoves_Compiled` / `sTerrainToType_Compiled`** are used.

## Runtime wiring

- **Refresh:** **`firered_portable_rom_battle_terrain_tables_refresh_after_rom_load()`**
  from **`engine_runtime_backend.c`** (after **`gBattleMoves`** ROM refresh).
- **Consumers:** **`NaturePowerMovesSelectTable()`** / **`TerrainToTypeSelectTable()`**
  in **`battle_script_commands.c`** (**`src`** and **`src_transformed`**).

## Non-portable builds

No ROM reads; tables remain static rodata.
