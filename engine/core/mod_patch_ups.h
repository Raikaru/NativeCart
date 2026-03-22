#ifndef ENGINE_MOD_PATCH_UPS_H
#define ENGINE_MOD_PATCH_UPS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Apply a UPS1 patch in memory (Alcaro Flips libups semantics).
 * - source: clean ROM bytes (not modified on disk).
 * - patch: full .ups file contents.
 * On success: *out_rom is malloc'd; caller must free().
 * err / err_cap: optional human-readable message (NUL-terminated if err_cap > 0).
 * Returns 1 on success, 0 on failure.
 */
int mod_patch_ups_apply(
    const uint8_t *source,
    size_t source_len,
    const uint8_t *patch,
    size_t patch_len,
    uint8_t **out_rom,
    size_t *out_len,
    char *err,
    size_t err_cap);

#endif /* ENGINE_MOD_PATCH_UPS_H */
