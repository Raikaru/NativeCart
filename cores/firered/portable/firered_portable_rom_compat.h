#ifndef GUARD_FIRERED_PORTABLE_ROM_COMPAT_H
#define GUARD_FIRERED_PORTABLE_ROM_COMPAT_H

#include "global.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Generic FireRed ROM hack / layout compatibility snapshot built on top of
 * firered_portable_rom_queries (CRC, cart title, game code, version).
 *
 * - Refreshed after each ROM load (see engine_backend_init).
 * - Optional one-line trace: FIRERED_TRACE_ROM_COMPAT=1
 * - Known profiles are a tiny, local table (CRC and/or title/game-code); extend
 *   in firered_portable_rom_compat.c — not a remote catalog.
 */

typedef enum {
    FIRERED_ROM_COMPAT_KIND_VANILLA_FIRE_RED = 0,
    FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK,
    FIRERED_ROM_COMPAT_KIND_UNKNOWN_PATCHED,
    FIRERED_ROM_COMPAT_KIND_NONSTANDARD,
} FireredRomCompatKind;

#define FIRERED_ROM_COMPAT_FLAG_VANILLA_LIKE (1u << 0)
#define FIRERED_ROM_COMPAT_FLAG_EXPANDED_ROM (1u << 1)
#define FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE (1u << 2)
#define FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS (1u << 3)
#define FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE (1u << 4)

typedef struct FireredRomCompatInfo {
    FireredRomCompatKind kind;
    uint32_t flags;
    const char *profile_id;
    char display_label[72];
    uint32_t rom_crc32;
    size_t rom_size;
    u8 game_code[4];
    u8 cart_title[12];
    u8 software_version;
} FireredRomCompatInfo;

/* Call after ROM is mapped and firered_portable_rom_queries_invalidate_cache(). */
void firered_portable_rom_compat_refresh_after_rom_load(void);

/* Cached snapshot from the last refresh; never NULL (stub on non-PORTABLE). */
const FireredRomCompatInfo *firered_portable_rom_compat_get(void);

/* Fill buf with a single-line summary (for launchers / debug); returns buf. */
char *firered_portable_rom_compat_format_summary(char *buf, size_t buf_size);

/*
 * Policy for optional ROM LZ / asset scans vs compiled portable mirrors.
 * FALSE only for vanilla 16 MiB retail-like FireRed (saves work; mirrors match).
 * TRUE for expanded, unknown patched, known hacks, nonstandard, etc.
 * Override: FIRERED_ROM_ASSET_SCAN_FORCE=1 (non-empty, not "0").
 */
bool8 firered_portable_rom_compat_should_attempt_rom_asset_scan(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_COMPAT_H */
