#include "global.h"

#include "portable/firered_portable_rom_asset_bind.h"

#include "portable/firered_portable_rom_compat.h"
#include "engine_internal.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_asset_trace_init(FireredRomAssetTrace *t, const char *trace_env_var, const char *prefix)
{
    (void)trace_env_var;
    (void)prefix;
    if (t != NULL)
    {
        t->on = 0;
        t->prefix = "";
    }
}

void firered_portable_rom_asset_trace_printf(const FireredRomAssetTrace *t, const char *fmt, ...)
{
    (void)t;
    (void)fmt;
}

void firered_portable_rom_asset_trace_vprintf(const FireredRomAssetTrace *t, const char *fmt, va_list ap)
{
    (void)t;
    (void)fmt;
    (void)ap;
}

void firered_portable_rom_asset_trace_flush(void)
{
}

int firered_portable_rom_asset_parse_hex_env(const char *name, size_t *out)
{
    (void)name;
    (void)out;
    return 0;
}

int firered_portable_rom_asset_env_nonempty(const char *name)
{
    (void)name;
    return 0;
}

bool8 firered_portable_rom_asset_want_signature_scan(bool8 use_compat_scan_gate, const char *force_scan_env_var)
{
    (void)use_compat_scan_gate;
    (void)force_scan_env_var;
    return FALSE;
}

bool8 firered_portable_rom_asset_bind_run(const FireredRomAssetBindParams *p)
{
    (void)p;
    return FALSE;
}

#else

extern char *getenv(const char *name);

void firered_portable_rom_asset_trace_init(FireredRomAssetTrace *t, const char *trace_env_var, const char *prefix)
{
    const char *e;

    if (t == NULL)
        return;

    t->prefix = (prefix != NULL) ? prefix : "";
    if (trace_env_var == NULL)
    {
        t->on = 0;
        return;
    }

    e = getenv(trace_env_var);
    t->on = (e != NULL && e[0] != '\0' && strcmp(e, "0") != 0);
}

void firered_portable_rom_asset_trace_vprintf(const FireredRomAssetTrace *t, const char *fmt, va_list ap)
{
    if (t == NULL || !t->on || fmt == NULL)
        return;

    fprintf(stderr, "%s ", t->prefix);
    vfprintf(stderr, fmt, ap);
}

void firered_portable_rom_asset_trace_printf(const FireredRomAssetTrace *t, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    firered_portable_rom_asset_trace_vprintf(t, fmt, ap);
    va_end(ap);
}

void firered_portable_rom_asset_trace_flush(void)
{
    fflush(stderr);
}

int firered_portable_rom_asset_parse_hex_env(const char *name, size_t *out)
{
    const char *e;
    char *end = NULL;
    unsigned long v;

    if (name == NULL || out == NULL)
        return 0;

    e = getenv(name);
    if (e == NULL || e[0] == '\0')
        return 0;

    v = strtoul(e, &end, 0);
    if (end == e)
        return 0;

    *out = (size_t)v;
    return 1;
}

int firered_portable_rom_asset_env_nonempty(const char *name)
{
    const char *e;

    if (name == NULL)
        return 0;
    e = getenv(name);
    return e != NULL && e[0] != '\0';
}

bool8 firered_portable_rom_asset_want_signature_scan(bool8 use_compat_scan_gate, const char *force_scan_env_var)
{
    const char *e;

    if (force_scan_env_var != NULL)
    {
        e = getenv(force_scan_env_var);
        if (e != NULL && e[0] != '\0' && strcmp(e, "0") != 0)
            return TRUE;
    }

    if (!use_compat_scan_gate)
        return TRUE;

    return firered_portable_rom_compat_should_attempt_rom_asset_scan();
}

bool8 firered_portable_rom_asset_bind_run(const FireredRomAssetBindParams *p)
{
    FireredRomAssetTrace tr;
    const u8 *rom;
    size_t rom_size;
    size_t min_sz;
    bool8 ok_env;
    bool8 want_scan;

    if (p == NULL)
        return FALSE;

    firered_portable_rom_asset_trace_init(&tr, p->trace_env_var, p->trace_prefix);

    min_sz = p->min_rom_size;
    if (min_sz == 0u)
        min_sz = 0x200u;

    rom_size = engine_memory_get_loaded_rom_size();
    if (rom_size < min_sz)
    {
        if (p->trace_rom_too_small != NULL)
            p->trace_rom_too_small(p->user, rom_size, min_sz, &tr);
        else
            firered_portable_rom_asset_trace_printf(&tr, "bind aborted: ROM not mapped / too small (need >= 0x%zx)\n",
                min_sz);
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;

    if (p->trace_rom_mapped != NULL)
        p->trace_rom_mapped(p->user, rom_size, &tr);

    ok_env = FALSE;
    if (p->try_env != NULL)
        ok_env = p->try_env(p->user, rom, rom_size, &tr);

    if (ok_env)
        return TRUE;

    if (p->clear_outputs != NULL)
        p->clear_outputs(p->user);

    want_scan = firered_portable_rom_asset_want_signature_scan(p->use_compat_scan_gate, p->force_scan_env_var);

    if (!want_scan)
    {
        if (p->trace_skip_scan_line != NULL && tr.on)
        {
            firered_portable_rom_asset_trace_printf(&tr, "%s", p->trace_skip_scan_line);
            firered_portable_rom_asset_trace_flush();
        }
        return FALSE;
    }

    if (p->trace_scan_beginning != NULL)
        p->trace_scan_beginning(p->user, rom_size, &tr);

    if (p->try_scan != NULL && p->try_scan(p->user, rom, rom_size, &tr))
        return TRUE;

    if (p->trace_fallback_line != NULL && tr.on)
    {
        firered_portable_rom_asset_trace_printf(&tr, "%s", p->trace_fallback_line);
        firered_portable_rom_asset_trace_flush();
    }

    return FALSE;
}

#endif /* PORTABLE */
