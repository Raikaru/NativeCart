#ifndef GUARD_FIRERED_PORTABLE_ROM_ASSET_BIND_H
#define GUARD_FIRERED_PORTABLE_ROM_ASSET_BIND_H

#include "global.h"

#include <stddef.h>
#include <stdarg.h>

/*
 * Small shared **flow** for ROM-backed portable asset families (FireRed hacks).
 *
 * Owns: ROM size gate, optional trace (env-gated), env-vs-scan ordering,
 * compat-gated signature scan policy, optional clear before scan, fallback hints.
 *
 * Does **not** own: per-family signatures, LZ layout, validation, or output layout —
 * those stay in family callbacks.
 */

typedef struct FireredRomAssetTrace {
    int on;
    const char *prefix;
} FireredRomAssetTrace;

void firered_portable_rom_asset_trace_init(FireredRomAssetTrace *t, const char *trace_env_var, const char *prefix);

void firered_portable_rom_asset_trace_printf(const FireredRomAssetTrace *t, const char *fmt, ...);
void firered_portable_rom_asset_trace_vprintf(const FireredRomAssetTrace *t, const char *fmt, va_list ap);

void firered_portable_rom_asset_trace_flush(void);

/* strtoul(..., 0); returns 1 on success. */
int firered_portable_rom_asset_parse_hex_env(const char *name, size_t *out);

int firered_portable_rom_asset_env_nonempty(const char *name);

/*
 * Signature scan policy:
 * - If force_scan_env_var is set and getenv is non-empty and not "0" → TRUE.
 * - Else if !use_compat_scan_gate → TRUE (always try scan after env fails).
 * - Else → firered_portable_rom_compat_should_attempt_rom_asset_scan().
 */
bool8 firered_portable_rom_asset_want_signature_scan(bool8 use_compat_scan_gate, const char *force_scan_env_var);

typedef struct FireredRomAssetBindParams {
    const char *trace_env_var;
    const char *trace_prefix;
    /* 0 → default 0x200 */
    size_t min_rom_size;
    void *user;

    void (*clear_outputs)(void *user);

    /* Return TRUE when outputs are fully bound for this family. */
    bool8 (*try_env)(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr);

    /* Return TRUE when scan produced a full bind. */
    bool8 (*try_scan)(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr);

    /* Optional: e.g. log rom_size / ENGINE_ROM after size gate. */
    void (*trace_rom_mapped)(void *user, size_t rom_size, FireredRomAssetTrace *tr);

    /*
     * Optional: if ROM is smaller than min_rom_size, call this instead of the
     * default too-small trace line (still only when tr->on).
     */
    void (*trace_rom_too_small)(void *user, size_t rom_size, size_t min_sz, FireredRomAssetTrace *tr);

    /* Optional: right before try_scan when scan is allowed. */
    void (*trace_scan_beginning)(void *user, size_t rom_size, FireredRomAssetTrace *tr);

    /* Optional one-line messages (include \n if desired); only if trace on. */
    const char *trace_skip_scan_line;
    const char *trace_fallback_line;

    bool8 use_compat_scan_gate;
    const char *force_scan_env_var;
} FireredRomAssetBindParams;

/*
 * Runs: size gate → [optional trace_rom_too_small or default] → optional
 * trace_rom_mapped → try_env → clear_outputs → want_signature_scan → [skip trace]
 * or trace_scan_beginning + try_scan → optional fallback trace.
 *
 * Returns TRUE if try_env or try_scan reported a full bind.
 */
bool8 firered_portable_rom_asset_bind_run(const FireredRomAssetBindParams *p);

#endif /* GUARD_FIRERED_PORTABLE_ROM_ASSET_BIND_H */
