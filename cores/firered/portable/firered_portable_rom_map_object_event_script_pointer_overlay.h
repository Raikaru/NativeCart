#ifndef GUARD_FIRERED_PORTABLE_ROM_MAP_OBJECT_EVENT_SCRIPT_POINTER_OVERLAY_H
#define GUARD_FIRERED_PORTABLE_ROM_MAP_OBJECT_EVENT_SCRIPT_POINTER_OVERLAY_H

#include "global.h"

/*
 * Optional ROM-backed u32[] parallel to gFireredPortableMapObjectEventScriptPtrs[] for portable
 * script pointer tokens 0x85000000 + index.
 *
 * Unlike the 0x81 event table, this list is **only** unique map object EventScript symbols
 * (sorted), with **no** ScrCmd dispatch prefix — every slot is a normal script pointer.
 * FIRERED_PORTABLE_MAP_OBJECT_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX stays 0 unless a future
 * generator change adds a non-ROM prefix (see docs/portable_rom_map_object_event_script_pointer_overlay.md).
 */

#define FIRERED_PORTABLE_MAP_OBJECT_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX 0u

const u8 *firered_portable_rom_map_object_event_script_pointer_overlay_ptr(u32 token_index);

#endif /* GUARD_FIRERED_PORTABLE_ROM_MAP_OBJECT_EVENT_SCRIPT_POINTER_OVERLAY_H */
