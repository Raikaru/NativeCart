#ifndef ENGINE_MOD_PATCH_FORMAT_H
#define ENGINE_MOD_PATCH_FORMAT_H

/*
 * Runtime mod / patch format identifiers.
 * BPS: mod_patch_bps.c — UPS: mod_patch_ups.c.
 * Same apply contract (malloc output, optional err string); SDL shell dispatches
 * by extension / argv (--bps vs --ups).
 */
enum ModPatchFormat {
    MOD_PATCH_FORMAT_NONE = 0,
    MOD_PATCH_FORMAT_BPS,
    MOD_PATCH_FORMAT_UPS,
};

#endif /* ENGINE_MOD_PATCH_FORMAT_H */
