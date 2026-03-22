# ROM-backed species → cry ID table (PORTABLE)

## Purpose

`SpeciesToCryId` (0-based internal species index, after `PlayCry`’s `species--`) uses **`sHoennSpeciesIdToCryId[]`** from `src/data/pokemon/cry_ids.h` for indices **`>= SPECIES_OLD_UNOWN_Z`** (i.e. Hoenn-era species through CHIMECHO). The C source uses **designated initializers**; in ROM the same data is a **contiguous `u16[]`** in species order.

Optional ROM load lets hacks that **remap cries** or **relocate** this table use the **cartridge** layout while keeping the same **`NUM_SPECIES`** as the build.

## Contiguous layout

- **Count:** `FIRERED_ROM_SPECIES_CRY_ID_TABLE_ENTRY_COUNT` = **`(NUM_SPECIES - 1) - SPECIES_OLD_UNOWN_Z`** (e.g. FireRed: **135** entries).
- **Entry `i`:** cry table index for 0-based species index **`SPECIES_OLD_UNOWN_Z + i`** (first entry = **TREECKO**).
- **Encoding:** little-endian **`u16`** per entry (same values as **`CRY_*`** constants in `include/constants/hoenn_cries.h`).

This matches `cry_ids.h`’s `HOENN_MON_SPECIES_START` (**277**) = **`SPECIES_OLD_UNOWN_Z + 1`**.

## Configuration

- **`FIRERED_ROM_SPECIES_CRY_ID_TABLE_OFF`** — hex file offset into the mapped ROM.
- **Profile rows** in `firered_portable_rom_species_cry_id_table.c` (`FireredRomU32TableProfileRow`), env first.

## Behaviour

Unconfigured or OOB → **compiled** `sHoennSpeciesIdToCryId` (vanilla). First two branches of `SpeciesToCryId` (Kanto / OLD_UNOWN) are **unchanged**.

## Related

- `cores/firered/portable/firered_portable_rom_species_cry_id_table.{h,c}`
- `docs/portable_rom_species_national_dex_table.md`, `docs/portable_rom_species_hoenn_dex_tables.md`
