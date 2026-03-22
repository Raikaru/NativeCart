#ifndef GUARD_FIRERED_PORTABLE_ROM_SPECIES_NATIONAL_DEX_H
#define GUARD_FIRERED_PORTABLE_ROM_SPECIES_NATIONAL_DEX_H

#include "global.h"

/*
 * Optional ROM-backed copy of the decomp's sSpeciesToNationalPokedexNum[]:
 * (NUM_SPECIES - 1) consecutive little-endian u16 values, species 1 .. NUM_SPECIES-1.
 *
 * Env: FIRERED_ROM_SPECIES_TO_NATIONAL_DEX_OFF (hex file offset into mapped ROM).
 * Profile: FireredRomU32TableProfileRow rows (rom_crc32 / profile_id) like other ROM tables.
 *
 * When inactive, SpeciesToNationalPokedexNum / NationalPokedexNumToSpecies use compiled rodata.
 */

void firered_portable_rom_species_national_dex_refresh_after_rom_load(void);

/* Non-NULL => forward table active (pointer is stable until next ROM load). */
const u16 *firered_portable_rom_species_national_dex_table(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_SPECIES_NATIONAL_DEX_H */
