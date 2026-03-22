# ROM-backed Hoenn dex tables (PORTABLE)

## Purpose

Complements **`docs/portable_rom_species_national_dex_table.md`**. On PORTABLE, **`sSpeciesToHoennPokedexNum[]`** and **`sHoennToNationalOrder[]`** in `pokemon.c` can be overridden from the mapped ROM when offsets are configured.

These drive **Hoenn vs National** dex ordering (summary numbers, `NationalToHoennOrder`, `HoennToNationalOrder`, `SpeciesToHoennPokedexNum`, inverse lookup via `HoennPokedexNumToSpecies`).

## Layout (each table)

- **Length:** `NUM_SPECIES - 1` little-endian **`u16`** values.
- **Table A — species → Hoenn summary number:** same order as decomp **`sSpeciesToHoennPokedexNum`** (species `1` … `NUM_SPECIES - 1`).
- **Table B — Hoenn order slot → National number:** same order as decomp **`sHoennToNationalOrder`** (Hoenn list positions `1` … `NUM_SPECIES - 1`).

## Configuration

| Env var | Table |
|--------|--------|
| **`FIRERED_ROM_SPECIES_TO_HOENN_DEX_OFF`** | Species → Hoenn dex index |
| **`FIRERED_ROM_HOENN_TO_NATIONAL_ORDER_OFF`** | Hoenn ordering slot → National dex # |

Each blob is **optional**. If only one env is set, the other table stays **compiled**.

Profile rows live in `firered_portable_rom_species_hoenn_dex_tables.c` (same `FireredRomU32TableProfileRow` pattern as other ROM tables). Env wins over profile.

## Behaviour

- **`firered_portable_rom_species_hoenn_dex_tables_refresh_after_rom_load()`** runs after ROM map (with other portable refresh hooks).
- Bounds checks mirror the National dex slice (`FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES`, full table must fit in the loaded ROM image).

## Related

- `cores/firered/portable/firered_portable_rom_species_hoenn_dex_tables.{h,c}`
- `docs/portable_rom_species_national_dex_table.md`
- `docs/portable_rom_species_cry_id_table.md` — **`SpeciesToCryId`** / **`cry_ids.h`**
