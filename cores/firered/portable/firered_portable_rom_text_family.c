#include "global.h"

#include "portable/firered_portable_rom_text_family.h"

#include "characters.h"

#include "engine_internal.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

size_t firered_portable_rom_text_byte_len_inclusive(const u8 *s)
{
    size_t n = 0;

    if (s == NULL)
        return 0u;
    while (s[n] != EOS)
        n++;
    return n + 1u;
}

void firered_portable_rom_text_family_bind_all(const FireredRomTextFamilyParams *params, const u8 **out_cache)
{
    unsigned i;

    if (params == NULL || out_cache == NULL || params->get_fallback == NULL)
        return;

    for (i = 0u; i < params->entry_count; i++)
        out_cache[i] = params->get_fallback(i, params->user);
}

#else

#include "portable/firered_portable_rom_asset_bind.h"

extern char *getenv(const char *name);

size_t firered_portable_rom_text_byte_len_inclusive(const u8 *s)
{
    size_t n = 0;

    if (s == NULL)
        return 0u;
    while (s[n] != EOS)
        n++;
    return n + 1u;
}

static int trace_enabled(const FireredRomTextFamilyParams *p)
{
    const char *e;

    if (p == NULL || p->trace_env_var == NULL)
        return 0;
    e = getenv(p->trace_env_var);
    return e != NULL && e[0] != '\0' && strcmp(e, "0") != 0;
}

static int trace_bind_detail_enabled(const FireredRomTextFamilyParams *p)
{
    const char *e;

    if (p == NULL || p->trace_bind_detail_env_var == NULL)
        return 0;
    e = getenv(p->trace_bind_detail_env_var);
    return e != NULL && e[0] != '\0' && strcmp(e, "0") != 0;
}

static void trace_flush(void)
{
    fflush(stderr);
}

static void trace_bind_detail_line(const FireredRomTextFamilyParams *params, unsigned i, const u8 *resolved, int u_env,
    int u_prof, int u_scan, int u_fb, const u8 *rom, size_t rom_size)
{
    const char *src;
    const char *k;
    const char *pfx;
    size_t j;

    if (params == NULL || resolved == NULL)
        return;

    src = u_env ? "env" : (u_prof ? "profile" : (u_scan ? "rom_scan" : (u_fb ? "compiled_fallback" : "?")));
    k = (params->env_key_names != NULL && params->env_key_names[i] != NULL) ? params->env_key_names[i] : "(no env key)";
    pfx = (params->trace_prefix != NULL) ? params->trace_prefix : "[firered rom-tx-family]";

    fprintf(stderr, "%s bind_detail entry=%u key=%s src=%s ptr=%p", pfx, i, k, src, (const void *)resolved);
    if (rom != NULL && resolved >= rom && (size_t)(resolved - rom) < rom_size)
        fprintf(stderr, " rom_off=0x%zX", (size_t)(resolved - rom));
    else
        fprintf(stderr, " rom_off=(not-in-mapped-ROM)");

    fprintf(stderr, " head=");
    for (j = 0u; j < 32u; j++)
    {
        u8 b = resolved[j];

        fprintf(stderr, "%02X", b);
        if (b == EOS)
            break;
    }
    fprintf(stderr, "\n");
    trace_flush();
}

static int string_terminates_in_rom(const u8 *rom, size_t rom_size, size_t off, size_t cap)
{
    size_t i;

    if (rom == NULL || off >= rom_size || cap == 0u)
        return 0;
    for (i = off; i < rom_size && (i - off) < cap; i++)
    {
        if (rom[i] == EOS)
            return 1;
    }
    return 0;
}

static const u8 *scan_rom_exact_first(const u8 *rom, size_t rom_size, const u8 *needle, size_t needle_len,
    unsigned *multi_hit_out)
{
    size_t i;
    unsigned hits = 0;
    const u8 *first = NULL;

    if (multi_hit_out != NULL)
        *multi_hit_out = 0u;

    if (needle_len == 0u || needle_len > rom_size || rom == NULL)
        return NULL;

    for (i = 0; i + needle_len <= rom_size; i++)
    {
        if (memcmp(rom + i, needle, needle_len) == 0)
        {
            hits++;
            if (first == NULL)
                first = rom + i;
        }
    }

    if (multi_hit_out != NULL && hits > 1u)
        *multi_hit_out = hits;

    if (hits == 0u)
        return NULL;
    return first;
}

static const u8 *resolve_one(const FireredRomTextFamilyParams *p, unsigned idx, int *u_env, int *u_prof, int *u_scan,
    int *u_fb)
{
    const u8 *rom;
    size_t rom_size;
    const u8 *fb;
    size_t needle_len;
    size_t off = 0;
    const u8 *q;
    unsigned multi = 0;
    size_t eos_cap;
    const char *envk;

    if (u_env != NULL)
        *u_env = 0;
    if (u_prof != NULL)
        *u_prof = 0;
    if (u_scan != NULL)
        *u_scan = 0;
    if (u_fb != NULL)
        *u_fb = 0;

    if (p == NULL || idx >= p->entry_count || p->get_fallback == NULL)
        return NULL;

    fb = p->get_fallback(idx, p->user);
    if (fb == NULL)
        return NULL;

    needle_len = firered_portable_rom_text_byte_len_inclusive(fb);
    eos_cap = p->env_offset_max_eos_search;
    if (eos_cap == 0u)
        eos_cap = 2048u;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;

    envk = (p->env_key_names != NULL) ? p->env_key_names[idx] : NULL;
    if (envk != NULL && firered_portable_rom_asset_parse_hex_env(envk, &off))
    {
        if (string_terminates_in_rom(rom, rom_size, off, eos_cap))
        {
            if (u_env != NULL)
                *u_env = 1;
            return rom + off;
        }
    }

    if (p->try_profile_rom_off != NULL && p->try_profile_rom_off(idx, &off, p->user))
    {
        if (string_terminates_in_rom(rom, rom_size, off, eos_cap))
        {
            if (u_prof != NULL)
                *u_prof = 1;
            return rom + off;
        }
    }

    if (rom_size < p->min_rom_size_for_scan || needle_len < (size_t)p->scan_min_match_len)
    {
        if (u_fb != NULL)
            *u_fb = 1;
        return fb;
    }

    if (p->try_scan != NULL)
        q = p->try_scan(rom, rom_size, idx, fb, needle_len, &multi, p->user);
    else
        q = scan_rom_exact_first(rom, rom_size, fb, needle_len, &multi);

    if (q != NULL)
    {
        if (u_scan != NULL)
            *u_scan = 1;
        if (multi > 1u && p->trace_warn_multi_scan && trace_enabled(p) && p->trace_prefix != NULL)
        {
            fprintf(stderr, "%s entry=%u multi_hit=%u (using first); set env override if needed\n", p->trace_prefix, idx,
                multi);
            trace_flush();
        }
        return q;
    }

    if (u_fb != NULL)
        *u_fb = 1;
    return fb;
}

void firered_portable_rom_text_family_bind_all(const FireredRomTextFamilyParams *params, const u8 **out_cache)
{
    unsigned i;
    int env_n = 0, prof_n = 0, scan_n = 0, fb_n = 0;
    int u_env, u_prof, u_scan, u_fb;
    const u8 *rom = NULL;
    size_t rom_size = 0;
    int detail_on;

    if (params == NULL || out_cache == NULL || params->get_fallback == NULL || params->entry_count == 0u)
        return;

    detail_on = trace_bind_detail_enabled(params);
    if (detail_on)
    {
        rom_size = engine_memory_get_loaded_rom_size();
        rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    }

    for (i = 0u; i < params->entry_count; i++)
    {
        u_env = u_prof = u_scan = u_fb = 0;
        out_cache[i] = resolve_one(params, i, &u_env, &u_prof, &u_scan, &u_fb);
        if (u_env)
            env_n++;
        else if (u_prof)
            prof_n++;
        else if (u_scan)
            scan_n++;
        else if (u_fb)
            fb_n++;
        if (detail_on)
            trace_bind_detail_line(params, i, out_cache[i], u_env, u_prof, u_scan, u_fb, rom, rom_size);
        if (trace_enabled(params) && u_fb && params->trace_prefix != NULL)
        {
            const char *k = (params->env_key_names != NULL && params->env_key_names[i] != NULL) ? params->env_key_names[i]
                                                                                               : "(no env key)";
            fprintf(stderr, "%s compiled_fallback entry=%u key=%s\n", params->trace_prefix, i, k);
            trace_flush();
        }
    }

    if (trace_enabled(params) && params->trace_prefix != NULL)
    {
        fprintf(stderr, "%s bound: env=%d profile=%d rom_scan=%d compiled_fallback=%d (of %u)\n", params->trace_prefix,
            env_n, prof_n, scan_n, fb_n, params->entry_count);
        trace_flush();
    }
}

#endif /* PORTABLE */
