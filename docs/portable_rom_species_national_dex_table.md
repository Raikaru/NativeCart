# ROM-backed species → National Pokédex number table (PORTABLE)

## Purpose

On SDL / PORTABLE, `SpeciesToNationalPokedexNum` and `NationalPokedexNumToSpecies` normally use the **compiled** `sSpeciesToNationalPokedexNum[]` rodata from `pokemon.c` (same values as vanilla decomp). Hacks that **relocate** this table in ROM or **change national numbers** while keeping the **same species count** as the build can point the runtime at the **ROM image** instead.

This is a **structural data** seam (dense `u16[]`), not a pointer/token overlay.

## Layout

- **Length:** `NUM_SPECIES - 1` entries.
- **Order:** species `1` (first real species) through `NUM_SPECIES - 1`, same as decomp `sSpeciesToNationalPokedexNum`.
- **Encoding:** little-endian `u16` per entry (national Dex number for that species).

## Configuration

1. **`FIRERED_ROM_SPECIES_TO_NATIONAL_DEX_OFF`** — hex file offset ( `strtoul` syntax, same as other `FIRERED_ROM_*_OFF` vars) to the start of the blob in the **mapped** ROM.

2. **Profile rows** (optional) — same pattern as other ROM `u32[]` helpers: `FireredRomU32TableProfileRow` in `firered_portable_rom_species_national_dex.c` (`rom_crc32` / `profile_id` / `table_file_off`). Env wins over profile.

## Behaviour

- After each successful `engine_backend_init`, `firered_portable_rom_species_national_dex_refresh_after_rom_load()` copies the ROM slice into a **static cache** when the offset resolves and the blob fits in the loaded ROM.
- If no env/profile or the slice is out of range, the cache stays **inactive** and behaviour matches **compiled** tables (vanilla default).
- **Count mismatch:** the engine is still built with a fixed `NUM_SPECIES`. A ROM with a **different number of species** is **not** supported by this slice; that requires a larger migration (dynamic species count, generated data regeneration).

## Finding the offset (vanilla / self-built ROM)

On a pokefirered ELF, locate the symbol for `sSpeciesToNationalPokedexNum` (or equivalent rodata) and convert its **ROM address** to a **file offset** (subtract `0x08000000` for standard GBA mapping). For patched hacks, use the same logical table in **their** ROM layout.

## Related

- Hoenn dex pair: **`docs/portable_rom_species_hoenn_dex_tables.md`**
- `cores/firered/portable/firered_portable_rom_species_national_dex.{h,c}`
- `src_transformed/pokemon.c` — `SpeciesToNationalPokedexNum`, `NationalPokedexNumToSpecies`
- Shared offset resolution: `firered_portable_rom_u32_table_resolve_base_off` (env → profile)
