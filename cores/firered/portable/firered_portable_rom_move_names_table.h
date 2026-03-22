#ifndef GUARD_FIRERED_PORTABLE_ROM_MOVE_NAMES_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_MOVE_NAMES_TABLE_H

#include "global.h"

#ifndef PORTABLE

static inline void firered_portable_rom_move_names_table_refresh_after_rom_load(void)
{
}

#else

void firered_portable_rom_move_names_table_refresh_after_rom_load(void);

#endif

#endif /* GUARD_FIRERED_PORTABLE_ROM_MOVE_NAMES_TABLE_H */
