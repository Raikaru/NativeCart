#ifndef GUARD_FIRERED_PORTABLE_ROM_SPECIES_CRY_ID_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_SPECIES_CRY_ID_TABLE_H

#include "global.h"

#include "constants/species.h"

/*
 * Contiguous little-endian u16 table parallel to sHoennSpeciesIdToCryId[] / cry_ids.h:
 * entry i is the M4A cry table index for 0-based internal species index (SPECIES_OLD_UNOWN_Z + i).
 *
 * Entry count matches SpeciesToCryId's third branch:
 *   species in [SPECIES_OLD_UNOWN_Z, NUM_SPECIES - 2] (0-based indices).
 *
 * Same length as decomp: (NUM_SPECIES - 1) - SPECIES_OLD_UNOWN_Z.
 * (cry_ids.h uses HOENN_MON_SPECIES_START == SPECIES_TREECKO == SPECIES_OLD_UNOWN_Z + 1.)
 */

#define FIRERED_ROM_SPECIES_CRY_ID_TABLE_ENTRY_COUNT ((size_t)((NUM_SPECIES - 1u) - (u16)SPECIES_OLD_UNOWN_Z))

void firered_portable_rom_species_cry_id_table_refresh_after_rom_load(void);

const u16 *firered_portable_rom_species_cry_id_table(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_SPECIES_CRY_ID_TABLE_H */
