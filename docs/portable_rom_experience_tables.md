# ROM-backed experience tables (`gExperienceTables`, PORTABLE)

## Purpose

**`gExperienceTables`** holds cumulative EXP thresholds per **growth rate** and **level**. Game logic (level-up, exp bar, Rare Candy, etc.) must read through **`ExperienceTableGet(growthRate, level)`** so PORTABLE can optionally substitute a **ROM-backed** copy when the hack relocates or retunes this rodata.

## Layout (contract)

Matches **`src/data/pokemon/experience_tables.h`** / decomp:

- **Rows:** **6** — `GROWTH_MEDIUM_FAST` (0) through `GROWTH_SLOW` (5).
- **Columns:** **`MAX_LEVEL + 1`** — levels **0 … `MAX_LEVEL`** inclusive (**101** columns when `MAX_LEVEL` is 100).
- **Encoding:** row-major **little-endian `u32`**.
- **Total size:** **6 × 101 × 4 = 2424** bytes.
- **Validation:** each row must be **non-decreasing** by level; otherwise the ROM cache is **not** activated (compiled fallback).

## Configuration

- **`FIRERED_ROM_EXPERIENCE_TABLES_OFF`** — hex file offset into mapped ROM (via `firered_portable_rom_u32_table_resolve_base_off`).
- **Profile rows** in `firered_portable_rom_experience_tables.c` (same resolver pattern as other ROM `u32` table bases).

## Behaviour

- After ROM load, **`firered_portable_rom_experience_tables_refresh_after_rom_load()`** may fill a static cache and set **active** when offset, bounds, and monotonic checks pass.
- **`ExperienceTableGet`**: if active and indices in range → cached value; else **`gExperienceTables[growthRate][level]`**.
- Non-**PORTABLE** builds: **`ExperienceTableGet`** is a macro expanding to direct table access — **zero overhead**, vanilla unchanged.

## Related

- `cores/firered/portable/firered_portable_rom_experience_tables.{h,c}`
- `include/pokemon.h` — accessor declaration / macro
