#ifndef GUARD_FIRERED_PORTABLE_ROM_EXPERIENCE_TABLES_H
#define GUARD_FIRERED_PORTABLE_ROM_EXPERIENCE_TABLES_H

#include "global.h"

/*
 * Optional ROM copy of gExperienceTables: row-major little-endian u32.
 * Rows: GROWTH_MEDIUM_FAST (0) .. GROWTH_SLOW (5) — 6 rows.
 * Cols: level 0 .. MAX_LEVEL — (MAX_LEVEL + 1) columns.
 *
 * See docs/portable_rom_experience_tables.md.
 */

void firered_portable_rom_experience_tables_refresh_after_rom_load(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_EXPERIENCE_TABLES_H */
