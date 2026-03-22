#ifndef GUARD_FIRERED_PORTABLE_ROM_QUERIES_H
#define GUARD_FIRERED_PORTABLE_ROM_QUERIES_H

#include "global.h"

/*
 * Generic ROM introspection for portable builds: full-ROM CRC (IEEE / BPS-style,
 * poly 0xEDB88320) and cartridge header fields. Cached until the next
 * firered_portable_rom_queries_invalidate_cache() (called after each ROM load).
 *
 * Any hack-specific policy (e.g. “enable feature X when CRC ∈ list”) should
 * live in feature code or tooling, not here.
 */

void firered_portable_rom_queries_invalidate_cache(void);
u32 firered_portable_rom_crc32_ieee_full(void);

/* GBA internal name @ 0xA0 (12 bytes, space-padded). Returns bytes copied (12) or 0. */
u8 firered_portable_rom_queries_copy_cart_title12(u8 *out12);

/* Game code @ 0xAC (4 ASCII bytes, e.g. BPRE). Returns bytes copied (4) or 0. */
u8 firered_portable_rom_queries_copy_game_code4(u8 *out4);

/* Software version @ 0xBC. Returns 0 if ROM too small. */
u8 firered_portable_rom_queries_software_version(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_QUERIES_H */
