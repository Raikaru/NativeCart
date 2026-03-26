#ifndef GUARD_FIRERED_PORTABLE_ROM_EGG_MOVES_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_EGG_MOVES_TABLE_H

#include "global.h"

#ifndef PORTABLE

/* Include `data/pokemon/egg_moves.h` before this header (defines `gEggMoves`). */
extern const u16 gEggMoves[];

#define FireredEggMovesTable()           (gEggMoves)
#define FireredEggMovesTableWordCount()  ((u32)NELEMS(gEggMoves))
#define firered_portable_rom_egg_moves_table_refresh_after_rom_load() ((void)0)

#else

const u16 *FireredEggMovesTable(void);
u32 FireredEggMovesTableWordCount(void);
void firered_portable_rom_egg_moves_table_refresh_after_rom_load(void);

#endif

#endif /* GUARD_FIRERED_PORTABLE_ROM_EGG_MOVES_TABLE_H */
