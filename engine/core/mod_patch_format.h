#ifndef ENGINE_MOD_PATCH_FORMAT_H
#define ENGINE_MOD_PATCH_FORMAT_H

/*
 * Runtime mod / patch format identifiers.
 * BPS is implemented in mod_patch_bps.c.
 * UPS (or IPS) can be added as separate translation units implementing the same
 * apply contract as mod_patch_bps_apply() without changing the ROM load path
 * in engine/shells/sdl/main.c — only the resolver / dispatcher needs a format
 * switch (extension sniffing or explicit flag).
 */
enum ModPatchFormat {
    MOD_PATCH_FORMAT_NONE = 0,
    MOD_PATCH_FORMAT_BPS,
    /* MOD_PATCH_FORMAT_UPS, */
};

#endif /* ENGINE_MOD_PATCH_FORMAT_H */
