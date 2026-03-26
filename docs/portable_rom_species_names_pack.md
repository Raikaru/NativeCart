# Portable ROM: species display names pack (`gSpeciesNames`)

## Purpose

**`gSpeciesNames[species]`** (GBA-encoded, **`POKEMON_NAME_LENGTH + 1`** per row) drives UI/script copy for species labels. Portable builds can load a **flat ROM blob** matching the compiled grid in **`species_names_portable.h`**.

## Grid dimensions

- **Rows:** **`SPECIES_CHIMECHO + 1`** (**412** for vanilla), indices **`0 .. SPECIES_CHIMECHO`** (same designated-init coverage as **`gSpeciesNames_Compiled`**).
- **Stride:** **`POKEMON_NAME_LENGTH + 1`** (**11**).
- **Total bytes:** **412 × 11 = 4532**.

## Wire format

Raw row-major **`u8`** copy of **`gSpeciesNames_Compiled`**. Each row must contain at least one **`EOS`** (**`0xFF`**) byte within the stride (validation).

## Activation

- **`FIRERED_ROM_SPECIES_NAMES_PACK_OFF`**
- Profile: **`s_species_names_profile_rows`** in **`firered_portable_rom_species_names_pack.c`**.

**`PORTABLE`:** **`gSpeciesNames`** expands to **`FireredSpeciesNamesTable()`** (ROM buffer or **`gSpeciesNames_Compiled`**) so static initializers that need a constant base still work when they use **`gSpeciesNames_Compiled`** directly.

## Half-migration note

Species indices **above `SPECIES_CHIMECHO`** (e.g. Unown forms) are **outside** this grid in the vanilla portable snapshot; behavior is unchanged from the pre-pack layout.

## Related

- **`cores/firered/portable/firered_portable_rom_species_names_pack.{h,c}`**
- **`include/pokemon.h`**
- **`src_transformed/species_names_portable.h`** (`gSpeciesNames_Compiled`)
- Offline: **`tools/portable_generators/validate_species_names_rom_pack.py`** (`tools/docs/README_validators.md`)
