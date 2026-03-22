#ifndef GUARD_FIRERED_PORTABLE_ROM_EVENT_SCRIPT_POINTER_OVERLAY_H
#define GUARD_FIRERED_PORTABLE_ROM_EVENT_SCRIPT_POINTER_OVERLAY_H

#include "global.h"

/*
 * Optional ROM-backed u32[] parallel to gFireredPortableEventScriptPtrs[] for portable
 * script pointer tokens 0x81000000 + index.
 *
 * Indices 0 .. (FIRERED_PORTABLE_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX - 1) are ScrCmd_* host
 * dispatch pointers and are never read from ROM (see docs/portable_rom_event_script_pointer_overlay.md).
 */

/* Matches data/script_cmd_table.inc: opcodes 0x00–0xD4 inclusive (213 entries). */
#define FIRERED_PORTABLE_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX 213u

const u8 *firered_portable_rom_event_script_pointer_overlay_ptr(u32 token_index);

#endif /* GUARD_FIRERED_PORTABLE_ROM_EVENT_SCRIPT_POINTER_OVERLAY_H */
