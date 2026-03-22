#ifndef GUARD_FIRERED_PORTABLE_ROM_ASSET_PROFILE_H
#define GUARD_FIRERED_PORTABLE_ROM_ASSET_PROFILE_H

#include "global.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Generic **built-in** ROM asset offset hints for portable binders.
 *
 * Purpose: when signature scans are wrong or ambiguous (common on large hacks),
 * ship or fork-local **small** tables of known file offsets keyed by
 * `firered_portable_rom_compat_get()` (CRC32 + profile_id from the compat layer).
 *
 * Priority (each family binder implements this order):
 *   1) Per-family **environment** variables (highest)
 *   2) **Profile** row match from this module
 *   3) **Signature / LZ scan** (existing)
 *   4) Compiled *_Portable fallback
 *
 * This module does **not** validate blobs, scan ROM, or fetch remote data.
 * Add rows only in `firered_portable_rom_asset_profile.c` (or a tiny include
 * list kept next to it). Keep the table small.
 *
 * Row matching:
 *   - If rom_crc32 != 0, require loaded ROM CRC == rom_crc32.
 *   - If profile_id != NULL, require strcmp(compat.profile_id, profile_id) == 0.
 *   - If rom_crc32 == 0, CRC is ignored (profile_id-only or combined).
 *   - Rows with rom_crc32 == 0 and profile_id == NULL are skipped (invalid).
 */

typedef enum FireredRomAssetFamily {
    FIRERED_ROM_ASSET_FAMILY_TITLE_LOGO,
    FIRERED_ROM_ASSET_FAMILY_COPYRIGHT_PRESS_START,
    FIRERED_ROM_ASSET_FAMILY_OAK_MAIN_BG,
    FIRERED_ROM_ASSET_FAMILY_MAIN_MENU_PALS,
    FIRERED_ROM_ASSET_FAMILY_COUNT
} FireredRomAssetFamily;

#define FIRERED_ROM_ASSET_PROFILE_HAS_PAL (1u << 0)
#define FIRERED_ROM_ASSET_PROFILE_HAS_PAL2 (1u << 1)
#define FIRERED_ROM_ASSET_PROFILE_HAS_TILES (1u << 2)
#define FIRERED_ROM_ASSET_PROFILE_HAS_MAP (1u << 3)

typedef struct FireredRomAssetProfileOffsets {
    uint32_t set_mask;
    size_t pal_off;
    size_t pal2_off;
    size_t tiles_off;
    size_t map_off;
} FireredRomAssetProfileOffsets;

/*
 * On TRUE, *out is filled for fields present in set_mask; other fields are 0.
 * On FALSE, *out is zeroed.
 */
bool8 firered_portable_rom_asset_profile_lookup(FireredRomAssetFamily family, FireredRomAssetProfileOffsets *out);

#endif /* GUARD_FIRERED_PORTABLE_ROM_ASSET_PROFILE_H */
