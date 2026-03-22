#ifndef GUARD_FIRERED_PORTABLE_ROM_SPECIES_HOENN_DEX_TABLES_H
#define GUARD_FIRERED_PORTABLE_ROM_SPECIES_HOENN_DEX_TABLES_H

#include "global.h"

/*
 * Optional ROM copies of pokemon.c Hoenn dex tables (each (NUM_SPECIES - 1) x le u16):
 *   - species 1..NUM_SPECIES-1 -> Hoenn summary number (sSpeciesToHoennPokedexNum)
 *   - Hoenn order slot 1..NUM_SPECIES-1 -> National dex number (sHoennToNationalOrder)
 *
 * Env: FIRERED_ROM_SPECIES_TO_HOENN_DEX_OFF, FIRERED_ROM_HOENN_TO_NATIONAL_ORDER_OFF
 * Each table is optional independently; unset -> compiled fallback for that table only.
 */

void firered_portable_rom_species_hoenn_dex_tables_refresh_after_rom_load(void);

const u16 *firered_portable_rom_species_to_hoenn_dex_table(void);
const u16 *firered_portable_rom_hoenn_to_national_order_table(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_SPECIES_HOENN_DEX_TABLES_H */
