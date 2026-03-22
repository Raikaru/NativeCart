# Portable ROM: Battle Tower Pokémon templates

Compiled data: **`gBattleTowerLevel50Mons_Compiled[]`** and **`gBattleTowerLevel100Mons_Compiled[]`** (`src/data/battle_tower/level_50_mons.h`, `level_100_mons.h`). Each array has **300** entries of **`struct BattleTowerPokemonTemplate`** (species, Battle Tower held-item id, team flags, four move ids, EV spread bitmask, nature). Consumed from **`FillBattleTowerTrainerParty`** in **`src/battle_tower.c`** via **`gBattleTowerLevel50Mons`** / **`gBattleTowerLevel100Mons`** macros (**`include/battle_tower.h`**).

Portable access: **`gBattleTowerLevel50MonsActive`** / **`gBattleTowerLevel100MonsActive`**; when both are **non-NULL**, the macros resolve to the **host-mirrored** pools. Otherwise the compiled tables are used.

## Pack (`FIRERED_ROM_BATTLE_TOWER_MON_TEMPLATES_PACK_OFF`)

Env hex + optional profile row **`__firered_builtin_battle_tower_mon_templates_profile_none__`**.

| Offset | Size | Field |
|--------|------|--------|
| **0** | **4** | Magic **`0x42545450`** (`BTTP` LE) |
| **4** | **4** | Version **`1`** |
| **8** | **4** | **`count50`** — must equal **`BATTLE_TOWER_LEVEL50_MON_COUNT`** (**300** in vanilla) |
| **12** | **4** | **`count100`** — must equal **`BATTLE_TOWER_LEVEL100_MON_COUNT`** (**300** in vanilla) |
| **16** | **4** | **`row_bytes`** — must equal **`sizeof(struct BattleTowerPokemonTemplate)`** |
| **20** | **4** | **`reserved`** — must be **`0`** |
| **24** | **`count50 × row_bytes`** | Level-50 pool, row-major |
| *follows* | **`count100 × row_bytes`** | Level-100 pool, row-major |

**Wire row layout** matches the in-memory **`struct BattleTowerPokemonTemplate`** (little-endian **`u16`**, then **`u8`** fields as in the decomp).

**Validation:** **`reserved == 0`**; counts and **`row_bytes`** match the portable build’s compiled geometry; each row passes bounds checks (**`SPECIES_NONE < species < NUM_SPECIES`**, **`heldItem ≤ BATTLE_TOWER_ITEM_GANLON_BERRY`**, **`moves[i] < MOVES_COUNT`**, **`evSpread`** only uses **`F_EV_SPREAD_*`** bits, **`nature < NUM_NATURES`**). On any failure, pools are freed and both **`*Active`** pointers are **NULL** (compiled fallback).

**Refresh:** **`firered_portable_rom_battle_tower_mon_templates_refresh_after_rom_load()`** from **`engine_backend_init`**, immediately after move names.
