#ifndef GUARD_FIRERED_PORTABLE_ROM_TEXT_FAMILY_H
#define GUARD_FIRERED_PORTABLE_ROM_TEXT_FAMILY_H

#include "global.h"

#include <stddef.h>

/*
 * Generic **bind-once** resolution for a bounded set of FireRed GBA-encoded strings
 * (EOS-terminated) from the mapped ROM, with compiled decomp fallbacks.
 *
 * Resolution order **per entry** (family implements via firered_portable_rom_text_family_bind_all):
 *   1) Optional per-entry **environment** hex offset (`env_key_names[i]`)
 *   2) Optional **profile** offset (`try_profile_rom_off`)
 *   3) Optional **scan** (`try_scan` or built-in exact memcmp vs fallback bytes)
 *   4) **`get_fallback`**
 *
 * Family layer owns: entry indices, fallback pointers, env var names, scan rules.
 * This module owns: trace, env parse (via rom_asset_bind helper), EOS validation for
 * env/profile offsets, default full-ROM exact match scan, bind-once cache fill.
 *
 * Tracing: set `trace_env_var` non-empty and not "0" → one summary line after bind_all;
 * optional per-hit multi-scan warning when `trace_warn_multi_scan` is TRUE.
 */

typedef struct FireredRomTextFamilyParams {
    const char *trace_env_var;
    const char *trace_prefix;
    unsigned entry_count;
    /* entry_count elements; NULL = no env override for that index */
    const char *const *env_key_names;
    size_t min_rom_size_for_scan;
    /* Max bytes to search for EOS when validating env/profile offsets (0 → 2048). */
    size_t env_offset_max_eos_search;
    /*
     * Minimum inclusive byte length (with EOS) required before attempting default scan.
     * Reduces false positives on very short strings (e.g. 12 for Oak intro).
     */
    unsigned scan_min_match_len;

    const u8 *(*get_fallback)(unsigned entry_index, void *user);
    /* Optional: return TRUE and set *out_off when this ROM/profile has a known offset. */
    bool8 (*try_profile_rom_off)(unsigned entry_index, size_t *out_off, void *user);
    /*
     * Optional custom scan. If NULL, built-in scan: first exact memcmp of needle in ROM.
     * `multi_hit_out` may be filled when >1 hit (first hit still returned).
     */
    const u8 *(*try_scan)(const u8 *rom, size_t rom_size, unsigned entry_index, const u8 *needle,
        size_t needle_len, unsigned *multi_hit_out, void *user);

    void *user;
    /* If TRUE and trace on, log when built-in/custom scan sees multiple matches. */
    bool8 trace_warn_multi_scan;
} FireredRomTextFamilyParams;

/* Inclusive byte length including trailing EOS. */
size_t firered_portable_rom_text_byte_len_inclusive(const u8 *s);

/*
 * Fill out_cache[0..entry_count-1] once. Safe to call with NULL params (no-op).
 * On non-PORTABLE builds: every entry is get_fallback (no ROM).
 */
void firered_portable_rom_text_family_bind_all(const FireredRomTextFamilyParams *params, const u8 **out_cache);

#endif /* GUARD_FIRERED_PORTABLE_ROM_TEXT_FAMILY_H */
