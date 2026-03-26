#include "global.h"
#include "engine_internal.h"
#include "engine_backend.h"

#include "portable/firered_portable_rom_queries.h"
#include "portable/firered_portable_rom_compat.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_compat_refresh_after_rom_load(void)
{
}

static const FireredRomCompatInfo s_stub = {
    .kind = FIRERED_ROM_COMPAT_KIND_NONSTANDARD,
    .flags = 0u,
    .profile_id = "non_portable",
    .display_label = "ROM compat: not available (non-PORTABLE build)",
    .rom_crc32 = 0u,
    .rom_size = 0u,
    .game_code = { '?', '?', '?', '?' },
    .cart_title = { 0 },
    .software_version = 0u,
};

const FireredRomCompatInfo *firered_portable_rom_compat_get(void)
{
    return &s_stub;
}

char *firered_portable_rom_compat_format_summary(char *buf, size_t buf_size)
{
    if (buf == NULL || buf_size == 0u)
        return buf;
    snprintf(buf, buf_size, "%s", s_stub.display_label);
    return buf;
}

bool8 firered_portable_rom_compat_should_attempt_rom_asset_scan(void)
{
    return TRUE;
}

#else

extern char *getenv(const char *name);

static FireredRomCompatInfo s_info;
static u8 s_compat_valid;

typedef struct {
    uint32_t crc32;
    const char *game_code4;
    const char *title_needle;
    const char *profile_id;
    const char *label;
    FireredRomCompatKind kind;
    uint32_t flags_or;
} KnownProfileRow;

/*
 * Order: specific hacks before vanilla retail-like rows. CRC 0 = wildcard.
 * Extend this table locally; keep it small.
 */
static const KnownProfileRow s_known_profiles[] = {
    /* Repo offline fixtures (CRC32 = IEEE full image; see tools/build_offline_layout_prose_fixture.py). */
    {
        0x217A04B1u,
        NULL,
        NULL,
        "offline_layout_prose_fixture",
        "Offline test: layout companion + intro-prose pointer table (CI fixture)",
        FIRERED_ROM_COMPAT_KIND_NONSTANDARD,
        FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,
    },
    {
        0x33B8FD06u,
        NULL,
        NULL,
        "offline_layout_companion_fixture",
        "Offline test: layout companion splice only (CI fixture)",
        FIRERED_ROM_COMPAT_KIND_NONSTANDARD,
        FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,
    },
    {
        0x8D911746u,
        NULL,
        NULL,
        "omega_layout_test",
        "Local Fire Red Omega test image (layout companion)",
        FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK,
        FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS
            | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,
    },
    {
        0x3479F247u,
        NULL,
        NULL,
        "omega_layout_prose_test",
        "Local Fire Red Omega test image (layout companion + intro prose)",
        FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK,
        FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS
            | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,
    },
    /* FireRed & LeafGreen+ style: internal name usually contains FRLG */
    {
        0u,
        "BPRE",
        "FRLG",
        "frlg_plus",
        "Known FireRed-derived hack (FRLG+)",
        FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK,
        FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS
            | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,
    },
    {
        0u,
        "BPGE",
        "FRLG",
        "frlg_plus",
        "Known FireRed-derived hack (FRLG+)",
        FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK,
        FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS
            | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,
    },
    /* Pokémon Unbound (common internal title contains UNBO / UNBD) */
    {
        0u,
        "BPRE",
        "UNBO",
        "unbound",
        "Known FireRed-derived hack (Unbound)",
        FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK,
        FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS
            | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,
    },
    {
        0u,
        "BPRE",
        "UNBD",
        "unbound",
        "Known FireRed-derived hack (Unbound)",
        FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK,
        FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE | FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS
            | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE,
    },
    /* Retail / decomp stock-like US FireRed */
    {
        0u,
        "BPRE",
        "FIRE",
        "vanilla_fire_red_us",
        "Vanilla FireRed (BPRE)",
        FIRERED_ROM_COMPAT_KIND_VANILLA_FIRE_RED,
        FIRERED_ROM_COMPAT_FLAG_VANILLA_LIKE | FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE,
    },
    /* Retail US LeafGreen */
    {
        0u,
        "BPGE",
        "LEAF",
        "vanilla_leaf_green_us",
        "Vanilla LeafGreen (BPGE)",
        FIRERED_ROM_COMPAT_KIND_VANILLA_FIRE_RED,
        FIRERED_ROM_COMPAT_FLAG_VANILLA_LIKE | FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE,
    },
};

static int game_code_matches(const u8 *gc, const char *expect4)
{
    if (expect4 == NULL)
        return 1;
    return memcmp(gc, expect4, 4u) == 0;
}

static int title_contains_needle(const u8 *title12, const char *needle)
{
    char tmp[16];
    size_t i;

    if (needle == NULL)
        return 1;

    for (i = 0u; i < 12u; i++)
    {
        u8 c = title12[i];
        tmp[i] = (char)((c >= 32u && c <= 126u) ? c : ' ');
    }
    tmp[12] = '\0';

    return strstr(tmp, needle) != NULL;
}

static int is_bpre_family(const u8 *gc)
{
    return memcmp(gc, "BPRE", 4u) == 0 || memcmp(gc, "BPGE", 4u) == 0;
}

static void trace_compat_if_requested(void)
{
    const char *e = getenv("FIRERED_TRACE_ROM_COMPAT");

    if (e == NULL || e[0] == '\0' || e[0] == '0')
        return;

    {
        char line[192];
        snprintf(line, sizeof(line),
                 "RomCompat: id=%s kind=%u flags=0x%08x crc=0x%08X size=%zu ver=%u",
                 s_info.profile_id, (unsigned)s_info.kind, (unsigned)s_info.flags,
                 (unsigned)s_info.rom_crc32, s_info.rom_size, (unsigned)s_info.software_version);
        engine_backend_trace_external(line);
    }
}

void firered_portable_rom_compat_refresh_after_rom_load(void)
{
    size_t rom_size;
    u32 crc;
    u8 title[12];
    u8 gc[4];
    size_t i;
    uint32_t flags;

    s_compat_valid = 0;

    rom_size = engine_memory_get_loaded_rom_size();
    crc = firered_portable_rom_crc32_ieee_full();
    memset(title, 0, sizeof(title));
    memset(gc, 0, sizeof(gc));
    (void)firered_portable_rom_queries_copy_cart_title12(title);
    (void)firered_portable_rom_queries_copy_game_code4(gc);

    memset(&s_info, 0, sizeof(s_info));
    s_info.rom_crc32 = crc;
    s_info.rom_size = rom_size;
    memcpy(s_info.game_code, gc, sizeof(gc));
    memcpy(s_info.cart_title, title, sizeof(title));
    s_info.software_version = firered_portable_rom_queries_software_version();

    if (rom_size < 0xBDu)
    {
        s_info.kind = FIRERED_ROM_COMPAT_KIND_NONSTANDARD;
        s_info.profile_id = "invalid_or_tiny_rom";
        snprintf(s_info.display_label, sizeof(s_info.display_label),
                 "Non-standard ROM (image too small for GBA header)");
        s_info.flags = FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE;
        s_compat_valid = 1;
        trace_compat_if_requested();
        return;
    }

    flags = 0u;
    if (rom_size > 0x01000000u)
        flags |= FIRERED_ROM_COMPAT_FLAG_EXPANDED_ROM;

    for (i = 0u; i < NELEMS(s_known_profiles); i++)
    {
        const KnownProfileRow *row = &s_known_profiles[i];

        if (row->crc32 != 0u && row->crc32 != crc)
            continue;
        if (!game_code_matches(gc, row->game_code4))
            continue;
        if (!title_contains_needle(title, row->title_needle))
            continue;

        s_info.kind = row->kind;
        s_info.profile_id = row->profile_id;
        snprintf(s_info.display_label, sizeof(s_info.display_label), "%s", row->label);
        s_info.flags = flags | row->flags_or;

        /*
         * Retail FireRed/LeafGreen are 16 MiB; a larger image with a stock-like
         * header is almost always a derivative hack — do not mark vanilla-like.
         */
        if ((s_info.flags & FIRERED_ROM_COMPAT_FLAG_EXPANDED_ROM) != 0u
            && s_info.kind == FIRERED_ROM_COMPAT_KIND_VANILLA_FIRE_RED)
        {
            s_info.kind = FIRERED_ROM_COMPAT_KIND_UNKNOWN_PATCHED;
            s_info.profile_id = "expanded_stock_like_header";
            s_info.flags &= (uint32_t)~FIRERED_ROM_COMPAT_FLAG_VANILLA_LIKE;
            s_info.flags |= FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS
                | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE;
            snprintf(s_info.display_label, sizeof(s_info.display_label),
                     "Expanded ROM / non-vanilla layout (stock-like header)");
        }

        s_compat_valid = 1;
        trace_compat_if_requested();
        return;
    }

    /* No table hit: classify from game code + layout */
    if (is_bpre_family(gc))
    {
        s_info.kind = FIRERED_ROM_COMPAT_KIND_UNKNOWN_PATCHED;
        if (memcmp(gc, "BPRE", 4u) == 0)
            s_info.profile_id = "unknown_bpre_family";
        else
            s_info.profile_id = "unknown_bpge_family";

        if (flags & FIRERED_ROM_COMPAT_FLAG_EXPANDED_ROM)
        {
            s_info.flags = flags | FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS
                | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE;
            snprintf(s_info.display_label, sizeof(s_info.display_label),
                     "Unknown patched FireRed family · expanded ROM");
        }
        else
        {
            s_info.flags = flags;
            snprintf(s_info.display_label, sizeof(s_info.display_label),
                     "Unknown patched FireRed family");
        }
    }
    else
    {
        s_info.kind = FIRERED_ROM_COMPAT_KIND_NONSTANDARD;
        s_info.profile_id = "nonstandard_game_code";
        s_info.flags = flags | FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE;
        snprintf(s_info.display_label, sizeof(s_info.display_label),
                 "Non-standard game code (not BPRE/BPGE)");
    }

    s_compat_valid = 1;
    trace_compat_if_requested();
}

const FireredRomCompatInfo *firered_portable_rom_compat_get(void)
{
    if (!s_compat_valid)
        firered_portable_rom_compat_refresh_after_rom_load();
    return &s_info;
}

char *firered_portable_rom_compat_format_summary(char *buf, size_t buf_size)
{
    const FireredRomCompatInfo *p = firered_portable_rom_compat_get();

    if (buf == NULL || buf_size == 0u)
        return buf;

    snprintf(buf, buf_size, "%s [id=%s crc=0x%08X size=%zu]", p->display_label, p->profile_id,
             (unsigned)p->rom_crc32, p->rom_size);
    return buf;
}

bool8 firered_portable_rom_compat_should_attempt_rom_asset_scan(void)
{
    const char *e;
    const FireredRomCompatInfo *c;

    e = getenv("FIRERED_ROM_ASSET_SCAN_FORCE");
    if (e != NULL && e[0] != '\0' && strcmp(e, "0") != 0)
        return TRUE;

    c = firered_portable_rom_compat_get();
    if (c == NULL)
        return TRUE;

    if (c->kind == FIRERED_ROM_COMPAT_KIND_VANILLA_FIRE_RED
        && (c->flags & FIRERED_ROM_COMPAT_FLAG_VANILLA_LIKE) != 0u
        && (c->flags & FIRERED_ROM_COMPAT_FLAG_EXPANDED_ROM) == 0u)
        return FALSE;

    return TRUE;
}

#endif /* PORTABLE */
