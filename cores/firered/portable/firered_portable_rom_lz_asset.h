#ifndef GUARD_FIRERED_PORTABLE_ROM_LZ_ASSET_H
#define GUARD_FIRERED_PORTABLE_ROM_LZ_ASSET_H

#include "global.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Shared helpers for ROM-backed **GBA LZ77 (0x10)** compressed assets.
 * Used by title-screen binders and any future “signature + follow-up LZ” seams.
 *
 * Not tied to a single hack or screen — pass signatures and expected relaxed
 * decompressed sizes from each caller.
 */

typedef enum {
    FIRERED_ROM_LZ_FOLLOWUP_NONE = 0,
    FIRERED_ROM_LZ_FOLLOWUP_STRICT_SIG = 1,
    FIRERED_ROM_LZ_FOLLOWUP_RELAXED_DEC = 2,
} FireredRomLzFollowupKind;

/*
 * Walk a GBA LZ77 stream (type 0x10) and return total compressed byte length,
 * or 0 if invalid / truncated. Matches engine_lz77_uncomp behavior.
 */
size_t firered_portable_rom_lz77_compressed_size(const u8 *src, size_t avail_in);

/*
 * Within [region_start, region_start + region_byte_len), search at most
 * search_cap bytes for a follow-up LZ blob after a parent asset (e.g. tilemap
 * after tiles):
 *   1) strict: memcmp against strict_sig (length strict_sig_len), align step 4
 *   2) relaxed: LZ77 0x10 with declared decompressed size == relaxed_decomp_size
 *      and a parseable stream (clen > 0)
 */
const u8 *firered_portable_rom_find_lz_followup(
    const u8 *region_start,
    size_t region_byte_len,
    size_t search_cap,
    const u8 *strict_sig,
    size_t strict_sig_len,
    u32 relaxed_decomp_size,
    FireredRomLzFollowupKind *kind_out);

#endif /* GUARD_FIRERED_PORTABLE_ROM_LZ_ASSET_H */
