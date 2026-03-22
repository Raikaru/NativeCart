#ifndef GUARD_FIRERED_PORTABLE_ROM_HM_MOVES_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_HM_MOVES_TABLE_H

#include "global.h"

#include <stddef.h>

/*
 * Optional ROM copy of sHMMoves[]: little-endian u16 list terminated by 0xFFFF (HM_MOVES_END).
 * Retail FireRed: 8 HM move IDs + sentinel = 9 u16 total (see pokemon.c).
 *
 * Load validates that at least one 0xFFFF appears within the fixed window so consumers never scan unbounded.
 */

#define FIRERED_ROM_HM_MOVES_TABLE_U16_COUNT ((size_t)9u)

void firered_portable_rom_hm_moves_table_refresh_after_rom_load(void);

/* NULL => use compiled sHMMoves. */
const u16 *firered_portable_rom_hm_moves_table(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_HM_MOVES_TABLE_H */
