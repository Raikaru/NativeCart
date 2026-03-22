#ifndef GUARD_FIRERED_PORTABLE_ROM_WILD_ENCOUNTER_FAMILY_H
#define GUARD_FIRERED_PORTABLE_ROM_WILD_ENCOUNTER_FAMILY_H

#include "wild_encounter.h"

#ifndef PORTABLE

#define FireredWildMonHeadersTable() (gWildMonHeaders)

#else

const struct WildPokemonHeader *FireredWildMonHeadersTable(void);
void firered_portable_rom_wild_encounter_family_refresh_after_rom_load(void);

#endif

#endif /* GUARD_FIRERED_PORTABLE_ROM_WILD_ENCOUNTER_FAMILY_H */
