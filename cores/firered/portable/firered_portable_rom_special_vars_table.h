#ifndef GUARD_FIRERED_PORTABLE_ROM_SPECIAL_VARS_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_SPECIAL_VARS_TABLE_H

#include "global.h"

/*
 * Optional ROM-backed **gSpecialVars** pointer table (vanilla: 21× `u32` LE in ROM,
 * `data/event_scripts.s`: addresses of `gSpecialVar_0x8000` … `gSpecialVar_0x8014`).
 *
 * `slot_index` = `var_id - SPECIAL_VARS_START` (0 … SPECIAL_VARS_END - SPECIAL_VARS_START).
 *
 * Each ROM word is a GBA **EWRAM** address (must lie in ENGINE_EWRAM_*), 2-byte aligned.
 *
 * Priority: env `FIRERED_ROM_SPECIAL_VARS_TABLE_OFF` → built-in profile row → NULL
 * (caller uses compiled `gSpecialVars[slot]`).
 */

u16 *firered_portable_rom_special_var_ptr(u8 slot_index);

#endif /* GUARD_FIRERED_PORTABLE_ROM_SPECIAL_VARS_TABLE_H */
