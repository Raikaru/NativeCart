#ifndef GUARD_FIRERED_PORTABLE_ROM_DEOXYS_BASE_STATS_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_DEOXYS_BASE_STATS_TABLE_H

#include "global.h"

#include "constants/pokemon.h"

#include <stddef.h>

/*
 * Optional ROM copy of sDeoxysBaseStats[]: NUM_STATS (6) little-endian u16 values in
 * STAT_HP .. STAT_SPDEF order (same as pokemon.c for FIRERED Attack forme).
 *
 * Only affects GetDeoxysStat when species is SPECIES_DEOXYS and other guards pass.
 */

#define FIRERED_ROM_DEOXYS_BASE_STATS_TABLE_U16_COUNT ((size_t)NUM_STATS)

void firered_portable_rom_deoxys_base_stats_table_refresh_after_rom_load(void);

const u16 *firered_portable_rom_deoxys_base_stats_table(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_DEOXYS_BASE_STATS_TABLE_H */
