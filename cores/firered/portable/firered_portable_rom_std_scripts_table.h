#ifndef GUARD_FIRERED_PORTABLE_ROM_STD_SCRIPTS_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_STD_SCRIPTS_TABLE_H

#include "global.h"

#include <stddef.h>

/*
 * Optional ROM-backed **gStdScripts** pointer table (9 entries in vanilla FireRed:
 * STD_OBTAIN_ITEM … STD_RECEIVED_ITEM in data/event_scripts.s).
 *
 * When a file offset into the **mapped** ROM is known (env or tiny built-in profile),
 * `firered_portable_rom_std_script_ptr` reads `u32` little-endian GBA addresses from
 * `rom + table_off + 4 * stdIdx` and returns `(const u8 *)(uintptr_t)addr` when in-range.
 *
 * Otherwise returns NULL → callers keep using compiled `gStdScripts[stdIdx]`.
 *
 * Priority: env `FIRERED_ROM_STD_SCRIPTS_TABLE_OFF` → built-in profile row → NULL.
 */

const u8 *firered_portable_rom_std_script_ptr(u8 std_idx);

#endif /* GUARD_FIRERED_PORTABLE_ROM_STD_SCRIPTS_TABLE_H */
