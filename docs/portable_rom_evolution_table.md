# ROM-backed evolution table (`gEvolutionTable`, PORTABLE)

## Purpose

**`gEvolutionTable`** maps each species to up to **`EVOS_PER_MON` (5)** evolution entries (`struct Evolution`: **`method`**, **`param`**, **`targetSpecies`**). Portable SDL can mirror the **loaded ROM** when a contiguous image matches this build’s layout, so hacks that **retarget evolutions, levels, items, or trade rules** are reflected without recompiling.

## Layout contract (wire format)

- **Shape:** **`NUM_SPECIES` × `EVOS_PER_MON`** entries, **row-major** (species `0 … NUM_SPECIES-1`, then slots `0 … EVOS_PER_MON-1` per species).
- **Entry size:** **`6`** bytes = **3 × little-endian `u16`**: `method`, `param`, `targetSpecies` (same order as `struct Evolution` on the portable GCC build; **`sizeof(struct Evolution) == 6`**).
- **Total blob size:** **`NUM_SPECIES * EVOS_PER_MON * 6`** bytes (e.g. **12 360** when `NUM_SPECIES == 412` and `EVOS_PER_MON == 5`).

## Semantics (reference)

- **Unused slot:** **`method == 0`**, **`param == 0`**, **`targetSpecies == 0`**.
- **Active slot:** **`method`** is a FireRed evolution constant (**`EVO_FRIENDSHIP` … `EVO_BEAUTY`**, values **1–15** in `include/constants/pokemon.h`). **`param`** depends on the method (level threshold, item ID, etc.). **`targetSpecies`** is a valid species index (**`1 … NUM_SPECIES-1`**, not **`SPECIES_NONE`**).

## Validation (fail-safe)

1. **`sizeof(struct Evolution) == 6`** on the portable target (`_Static_assert` in module + header).
2. **`SPECIES_NONE`**: every slot is **(0, 0, 0)**.
3. **Per slot:** if **`method == 0`** then **`param`** and **`targetSpecies`** must be **0**; if **`method != 0`** then **`method`** is in **1…15** and **`targetSpecies`** is **`>= 1`** and **`< NUM_SPECIES`**.

Failures → ROM cache **not** activated; **`gEvolutionTableActive`** stays **NULL** and the **`gEvolutionTable`** macro uses **`gEvolutionTable_Compiled`**.

**Note:** ROMs that introduce **new evolution method IDs** without matching engine `switch` cases will not evolve anyway; if they also violate the **1…15** range, this validator will reject the blob. Extend the engine and relax validation together if needed.

## Runtime wiring

- **Env / profile base:** **`FIRERED_ROM_EVOLUTION_TABLE_OFF`** (hex file offset), with the same **profile row** pattern as other `firered_portable_rom_u32_table_resolve_base_off` users (placeholder profile id `__firered_builtin_evolution_profile_none__`).
- **Refresh:** **`firered_portable_rom_evolution_table_refresh_after_rom_load()`** (from **`engine_runtime_backend.c`** after other species-adjacent tables).
- **Access:** **`include/pokemon.h`** defines **`gEvolutionTable`** on **PORTABLE** as **`gEvolutionTableActive`** if non-**NULL**, else **`gEvolutionTable_Compiled`**. Compiled rodata lives in **`src/data/pokemon/evolution.h`** as **`gEvolutionTable_Compiled`**.

## Non-portable (GBA) builds

Unchanged symbol **`gEvolutionTable`**; no ROM refresh.
