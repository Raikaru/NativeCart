#ifndef GUARD_FIRERED_PORTABLE_ROM_LEVEL_UP_LEARNSETS_FAMILY_H
#define GUARD_FIRERED_PORTABLE_ROM_LEVEL_UP_LEARNSETS_FAMILY_H

#include "global.h"

#ifndef PORTABLE

static inline void firered_portable_rom_level_up_learnsets_family_refresh_after_rom_load(void)
{
}

#else

void firered_portable_rom_level_up_learnsets_family_refresh_after_rom_load(void);

#endif

#endif /* GUARD_FIRERED_PORTABLE_ROM_LEVEL_UP_LEARNSETS_FAMILY_H */
