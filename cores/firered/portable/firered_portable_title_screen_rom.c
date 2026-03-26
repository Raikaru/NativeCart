#include "portable/firered_portable_title_screen_rom.h"

#include "portable/firered_portable_rom_compat.h"
#include "portable/firered_portable_rom_asset_profile.h"
#include "portable/firered_portable_rom_asset_bind.h"
#include "portable/firered_portable_rom_lz_asset.h"
#include "../../../src_transformed/title_screen_portable_assets.h"
#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    TITLE_LOGO_TILE_SIG_LEN = 32,
    TITLE_LOGO_MAP_SIG_LEN = 32,
    TITLE_LOGO_PAL_SIZE = 512,
    /* Vanilla game_title_logo.8bpp.lz uncompressed size (8bpp tile sheet). */
    TITLE_LOGO_TILES_DECOMP_SIZE = 0x4000u,
    /* Vanilla game_title_logo.bin uncompressed size (tilemap). */
    TITLE_LOGO_MAP_DECOMP_SIZE = 0x500u,
    /*
     * Previously only ~8KiB after compressed tiles was scanned, with a strict
     * 32-byte match on the stock compressed map — too tight for hacks that pad
     * or reorder rodata (e.g. FRLG+). Widen search; validate relaxed hits by
     * LZ header + dec size + parseable stream.
     */
    TITLE_LOGO_MAP_SEARCH_MAX_AFTER_TILES = 4u * 1024u * 1024u,
    MAX_TILE_MATCHES = 24u,
};

enum {
    TITLE_LOGO_MAP_MATCH_NONE = 0,
    TITLE_LOGO_MAP_MATCH_STRICT_32 = 1,
    TITLE_LOGO_MAP_MATCH_LZ_DEC0x500 = 2,
};

enum {
    TITLE_LOGO_TILE_TRY_OK = 0,
    TITLE_LOGO_TILE_TRY_FAIL_LZ = 1,
    TITLE_LOGO_TILE_TRY_FAIL_PAL_ROOM = 2,
    TITLE_LOGO_TILE_TRY_FAIL_MAP = 3,
};

enum {
    COPYRIGHT_TILES_SIG_LEN = 32,
    COPYRIGHT_MAP_SIG_LEN = 32,
    /* Retail copyright_press_start.4bpp.lz declared decompressed size. */
    COPYRIGHT_TILES_DECOMP_SIZE = 0x800u,
    COPYRIGHT_MAP_DECOMP_SIZE = 0x500u,
    COPYRIGHT_MAP_SEARCH_MAX_AFTER_TILES = 256u * 1024u,
    MAX_COPYRIGHT_TILE_MATCHES = 16u,
};

enum {
    COPYRIGHT_TILE_TRY_OK = 0,
    COPYRIGHT_TILE_TRY_FAIL_LZ = 1,
    COPYRIGHT_TILE_TRY_FAIL_MAP = 2,
};

enum {
    BOX_ART_PAL_SIZE = 32,
    BOX_ART_TILES_SIG_LEN = 32,
    BOX_ART_MAP_SIG_LEN = 32,
    BOX_ART_MAP_DECOMP_SIZE = 0x500u,
    BOX_ART_MAP_SEARCH_MAX_AFTER_TILES = 256u * 1024u,
    MAX_BOX_ART_TILE_MATCHES = 24u,
};

enum {
    BORDER_BG_TILES_SIG_LEN = 32,
    BORDER_BG_MAP_SIG_LEN = 32,
    BORDER_BG_MAP_DECOMP_SIZE = 0x500u,
    BORDER_BG_MAP_SEARCH_MAX_AFTER_TILES = 256u * 1024u,
    MAX_BORDER_BG_TILE_MATCHES = 24u,
};

enum {
    TITLE_EFFECT_SIG_LEN = 32,
    FLAMES_PAL_SIZE = 32,
};

enum {
    COPYRIGHT_MAP_MATCH_NONE = 0,
    COPYRIGHT_MAP_MATCH_STRICT_32 = 1,
    COPYRIGHT_MAP_MATCH_LZ_DEC0x500 = 2,
};

/* First bytes of game_title_logo tiles LZ (8bpp source); FIRERED/LeafGreen match in stock assets. */
static const u8 sTitleLogoTilesLzSig[TITLE_LOGO_TILE_SIG_LEN] = {
    0x10, 0x00, 0x40, 0x00, 0x3f, 0x00, 0x00, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01,
    0xf0, 0x01, 0xf0, 0x01, 0xff, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01,
    0xf0, 0x01
};

/* First bytes of game_title_logo.bin.lz */
static const u8 sTitleLogoMapLzSig[TITLE_LOGO_MAP_SIG_LEN] = {
    0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x04,
    0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x00, 0x08, 0x00, 0x09, 0x00, 0x0a, 0x00, 0x0b,
    0x00, 0x00
};

/* graphics/title_screen/copyright_press_start.4bpp.lz / .bin.lz (FireRed stock) */
static const u8 sCopyrightTilesLzSig[COPYRIGHT_TILES_SIG_LEN] = {
    0x10, 0x00, 0x08, 0x00, 0x39, 0x66, 0x66, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0xcc, 0xcc, 0x20,
    0x01, 0x7f, 0x6c, 0xf0, 0x19, 0x70, 0x1f, 0x10, 0x06, 0xf0, 0x19, 0xa0, 0x1f, 0xf0, 0x01, 0xf0,
};

static const u8 sCopyrightMapLzSig[COPYRIGHT_MAP_SIG_LEN] = {
    0x10, 0x00, 0x05, 0x00, 0x3d, 0x14, 0xf0, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0x50, 0x01, 0x13,
    0xf0, 0x01, 0xef, 0xf0, 0x01, 0xf0, 0x01, 0x60, 0x01, 0x00, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01,
};

static const u8 sBoxArtTilesLzSig[BOX_ART_TILES_SIG_LEN] = {
    0x10, 0xE0, 0x10, 0x00, 0x3C, 0x00, 0x00, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0x20, 0x01, 0xB0,
    0xBB, 0x60, 0xBB, 0xF0, 0x14, 0x20, 0x01, 0xBB, 0x00, 0xBB, 0xBB, 0xDD, 0x08, 0xBB, 0xDD, 0xDD,
};

static const u8 sBoxArtMapLzSig[BOX_ART_MAP_SIG_LEN] = {
    0x10, 0x00, 0x05, 0x00, 0x3F, 0x00, 0xD0, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0,
    0x01, 0xF0, 0x01, 0xFF, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01,
};

static const u8 sBorderBgTilesLzSig[BORDER_BG_TILES_SIG_LEN] = {
    0x10, 0x80, 0x00, 0x00, 0x33, 0xFF, 0xFF, 0xF0, 0x01, 0x90, 0x01, 0x66, 0x66, 0xF0, 0x01, 0xF0,
    0x01, 0x98, 0xF0, 0x01, 0xBB, 0xBB, 0xF0, 0x01, 0xF0, 0x01, 0xBB, 0xBB, 0x00, 0x00, 0x00, 0x00,
};

static const u8 sBorderBgMapLzSig[BORDER_BG_MAP_SIG_LEN] = {
    0x10, 0x00, 0x05, 0x00, 0x3F, 0x01, 0xE0, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0,
    0x01, 0xF0, 0x01, 0xFB, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0x70, 0x01, 0x02, 0xF0,
};

static const u8 sSlashGfxLzSig[TITLE_EFFECT_SIG_LEN] = {
    0x10, 0x00, 0x08, 0x00, 0x3F, 0x00, 0x00, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0,
    0x01, 0xF0, 0x01, 0xFF, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0x40, 0x01, 0x54, 0x01,
};

static const u8 sFlamesGfxLzSig[TITLE_EFFECT_SIG_LEN] = {
    0x10, 0x00, 0x05, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x30, 0x00, 0x02, 0x23, 0x20, 0x03, 0x20, 0x03,
    0x22, 0x22, 0x00, 0x22, 0x22, 0x06, 0x20, 0x03, 0x10, 0x0F, 0x20, 0x00, 0x03, 0x00, 0x22, 0x20,
};

static const u8 sBlankFlamesGfxLzSig[TITLE_EFFECT_SIG_LEN] = {
    0x10, 0x00, 0x05, 0x00, 0x3F, 0x00, 0x00, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0,
    0x01, 0xF0, 0x01, 0xFF, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xF0, 0x01,
};

static FireredTitleCoreProvenanceState s_title_core_provenance_game_title_logo =
    FIRERED_TITLE_CORE_PROVENANCE_UNCHANGED;
static FireredTitleCoreProvenanceState s_title_core_provenance_copyright_press_start =
    FIRERED_TITLE_CORE_PROVENANCE_UNCHANGED;

/*
 * One linear pass over ROM builds LZ77 (0x10) offset lists by declared dec size.
 * Heavy hacks (many 0x4000/0x500 blobs) otherwise pay repeated full-ROM scans for
 * each relaxed candidate + each map-anywhere lookup.
 */
enum { TITLE_ROM_LZ_INDEX_CAP = 2048 };

static const u8 *s_title_rom_lz_cache_rom;
static size_t s_title_rom_lz_cache_size;
static size_t s_title_lz_off_dec0x500[TITLE_ROM_LZ_INDEX_CAP];
static size_t s_title_lz_count_dec0x500;
static size_t s_title_lz_off_dec0x4000[TITLE_ROM_LZ_INDEX_CAP];
static size_t s_title_lz_count_dec0x4000;
static size_t s_title_lz_off_dec0x800[TITLE_ROM_LZ_INDEX_CAP];
static size_t s_title_lz_count_dec0x800;

static void title_rom_ensure_lz_indexes(const u8 *rom, size_t rom_size)
{
    size_t i;
    size_t n5 = 0u, n4 = 0u, n8 = 0u;

    if (rom == NULL || rom_size < 4u)
        return;
    if (s_title_rom_lz_cache_rom == rom && s_title_rom_lz_cache_size == rom_size)
        return;

    s_title_rom_lz_cache_rom = rom;
    s_title_rom_lz_cache_size = rom_size;
    s_title_lz_count_dec0x500 = 0u;
    s_title_lz_count_dec0x4000 = 0u;
    s_title_lz_count_dec0x800 = 0u;

    for (i = 0; i + 4u <= rom_size; i += 4u)
    {
        const u8 *p = rom + i;
        uint32_t dec;
        size_t clen;

        if (p[0] != 0x10)
            continue;
        dec = (uint32_t)p[1] | ((uint32_t)p[2] << 8) | ((uint32_t)p[3] << 16);
        clen = firered_portable_rom_lz77_compressed_size(p, rom_size - i);
        if (clen == 0)
            continue;

        if (dec == TITLE_LOGO_MAP_DECOMP_SIZE && n5 < TITLE_ROM_LZ_INDEX_CAP)
            s_title_lz_off_dec0x500[n5++] = i;
        if (dec == TITLE_LOGO_TILES_DECOMP_SIZE && n4 < TITLE_ROM_LZ_INDEX_CAP)
            s_title_lz_off_dec0x4000[n4++] = i;
        if (dec == COPYRIGHT_TILES_DECOMP_SIZE && n8 < TITLE_ROM_LZ_INDEX_CAP)
            s_title_lz_off_dec0x800[n8++] = i;
    }

    s_title_lz_count_dec0x500 = n5;
    s_title_lz_count_dec0x4000 = n4;
    s_title_lz_count_dec0x800 = n8;
}

static int ptr_in_rom_range(const u8 *rom, size_t rom_size, const u8 *ptr, size_t bytes)
{
    const u8 *end = rom + rom_size;

    if (ptr == NULL || ptr < rom)
        return 0;
    if ((size_t)(end - ptr) < bytes)
        return 0;
    return 1;
}

static const u8 *find_lz_by_signature(
    const u8 *rom, size_t rom_size, const u8 *sig, size_t sig_len)
{
    size_t i;

    for (i = 0; i + sig_len <= rom_size; i += 4)
    {
        size_t clen;
        if (memcmp(rom + i, sig, sig_len) != 0)
            continue;
        clen = firered_portable_rom_lz77_compressed_size(rom + i, rom_size - i);
        if (clen > 0)
            return rom + i;
    }
    return NULL;
}

static int title_logo_env_override_present(void)
{
    return firered_portable_rom_asset_env_nonempty("FIRERED_ROM_TITLE_LOGO_PAL_OFF")
        || firered_portable_rom_asset_env_nonempty("FIRERED_ROM_TITLE_LOGO_TILES_OFF")
        || firered_portable_rom_asset_env_nonempty("FIRERED_ROM_TITLE_LOGO_MAP_OFF");
}

static int title_copyright_env_override_present(void)
{
    return firered_portable_rom_asset_env_nonempty("FIRERED_ROM_COPYRIGHT_TILES_OFF")
        || firered_portable_rom_asset_env_nonempty("FIRERED_ROM_COPYRIGHT_MAP_OFF");
}

static int title_logo_bound_equals_baseline(
    const u8 *rom, size_t rom_size, const u8 *pal, const u8 *tiles, const u8 *map)
{
    size_t pal_size = sizeof(gGraphics_TitleScreen_GameTitleLogoPals_Portable);
    size_t tiles_size = sizeof(gGraphics_TitleScreen_GameTitleLogoTiles_Portable);
    size_t map_size = sizeof(gGraphics_TitleScreen_GameTitleLogoMap_Portable);

    if (!ptr_in_rom_range(rom, rom_size, pal, pal_size)
        || !ptr_in_rom_range(rom, rom_size, tiles, tiles_size)
        || !ptr_in_rom_range(rom, rom_size, map, map_size))
        return 0;

    return memcmp(pal, gGraphics_TitleScreen_GameTitleLogoPals_Portable, pal_size) == 0
        && memcmp(tiles, gGraphics_TitleScreen_GameTitleLogoTiles_Portable, tiles_size) == 0
        && memcmp(map, gGraphics_TitleScreen_GameTitleLogoMap_Portable, map_size) == 0;
}

static int title_copyright_bound_equals_baseline(
    const u8 *rom, size_t rom_size, const u8 *tiles, const u8 *map)
{
    size_t tiles_size = sizeof(gGraphics_TitleScreen_CopyrightPressStartTiles_Portable);
    size_t map_size = sizeof(gGraphics_TitleScreen_CopyrightPressStartMap_Portable);

    if (!ptr_in_rom_range(rom, rom_size, tiles, tiles_size) || !ptr_in_rom_range(rom, rom_size, map, map_size))
        return 0;

    return memcmp(tiles, gGraphics_TitleScreen_CopyrightPressStartTiles_Portable, tiles_size) == 0
        && memcmp(map, gGraphics_TitleScreen_CopyrightPressStartMap_Portable, map_size) == 0;
}

/*
 * Find title logo tilemap LZ after the compressed tiles blob.
 * Pass 1: stock 32-byte compressed prefix (vanilla / unchanged hacks).
 * Pass 2: LZ77 0x10 + declared decompressed size == 0x500 + full stream parses
 * (same dec size as retail map; works when repacking changes early bytes only).
 */
static const u8 *find_map_after_tiles(
    const u8 *rom, size_t rom_size, const u8 *tiles, size_t tiles_compressed_len, int *map_match_kind_out)
{
    const u8 *region_start = tiles + tiles_compressed_len;
    const u8 *rom_end = rom + rom_size;
    FireredRomLzFollowupKind k = FIRERED_ROM_LZ_FOLLOWUP_NONE;
    const u8 *map;

    if (map_match_kind_out != NULL)
        *map_match_kind_out = TITLE_LOGO_MAP_MATCH_NONE;

    if (region_start > rom_end)
        return NULL;

    map = firered_portable_rom_find_lz_followup(region_start, (size_t)(rom_end - region_start),
                                                TITLE_LOGO_MAP_SEARCH_MAX_AFTER_TILES, sTitleLogoMapLzSig,
                                                TITLE_LOGO_MAP_SIG_LEN, TITLE_LOGO_MAP_DECOMP_SIZE, &k);

    if (map_match_kind_out != NULL)
    {
        if (k == FIRERED_ROM_LZ_FOLLOWUP_STRICT_SIG)
            *map_match_kind_out = TITLE_LOGO_MAP_MATCH_STRICT_32;
        else if (k == FIRERED_ROM_LZ_FOLLOWUP_RELAXED_DEC)
            *map_match_kind_out = TITLE_LOGO_MAP_MATCH_LZ_DEC0x500;
    }

    return map;
}

static const u8 *find_map_anywhere_for_tiles(
    const u8 *rom, size_t rom_size, const u8 *tiles, int *map_match_kind_out)
{
    size_t j;
    const u8 *best = NULL;
    size_t best_distance = (size_t)-1;
    int best_kind = TITLE_LOGO_MAP_MATCH_NONE;
    int best_after = 0;

    title_rom_ensure_lz_indexes(rom, rom_size);

    for (j = 0; j < s_title_lz_count_dec0x500; j++)
    {
        size_t i = s_title_lz_off_dec0x500[j];
        const u8 *candidate = rom + i;
        int kind;
        int after;
        size_t distance;

        kind = (i + TITLE_LOGO_MAP_SIG_LEN <= rom_size
                && memcmp(candidate, sTitleLogoMapLzSig, TITLE_LOGO_MAP_SIG_LEN) == 0)
            ? TITLE_LOGO_MAP_MATCH_STRICT_32
            : TITLE_LOGO_MAP_MATCH_LZ_DEC0x500;
        after = (candidate >= tiles);
        distance = after ? (size_t)(candidate - tiles) : (size_t)(tiles - candidate);

        if (best == NULL
            || (kind == TITLE_LOGO_MAP_MATCH_STRICT_32 && best_kind != TITLE_LOGO_MAP_MATCH_STRICT_32)
            || (kind == best_kind && after && !best_after)
            || (kind == best_kind && after == best_after && distance < best_distance))
        {
            best = candidate;
            best_distance = distance;
            best_kind = kind;
            best_after = after;
        }
    }

    if (map_match_kind_out != NULL)
        *map_match_kind_out = best_kind;
    return best;
}

typedef struct TitleLogoTriple {
    const u8 *pal;
    const u8 *tiles;
    const u8 *map;
    int map_kind;
    int map_from_after;
    size_t map_distance;
    int tiles_strict_prefix;
    size_t cluster_span;
} TitleLogoTriple;

static int title_logo_try_tile_candidate(
    const u8 *rom, size_t rom_size, const u8 *tiles, TitleLogoTriple *out_ok)
{
    size_t tile_off = (size_t)(tiles - rom);
    size_t tiles_avail = rom_size - tile_off;
    size_t tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, tiles_avail);
    const u8 *pal;
    const u8 *map;
    int mk = TITLE_LOGO_MAP_MATCH_NONE;

    if (tiles_clen == 0)
        return TITLE_LOGO_TILE_TRY_FAIL_LZ;
    if (tile_off < TITLE_LOGO_PAL_SIZE)
        return TITLE_LOGO_TILE_TRY_FAIL_PAL_ROOM;

    pal = tiles - TITLE_LOGO_PAL_SIZE;
    map = find_map_after_tiles(rom, rom_size, tiles, tiles_clen, &mk);
    out_ok->map_from_after = (map != NULL);
    if (map == NULL)
    {
        map = find_map_anywhere_for_tiles(rom, rom_size, tiles, &mk);
        if (map == NULL)
            return TITLE_LOGO_TILE_TRY_FAIL_MAP;
    }

    out_ok->pal = pal;
    out_ok->tiles = tiles;
    out_ok->map = map;
    out_ok->map_kind = mk;
    out_ok->map_distance = (map >= tiles) ? (size_t)(map - tiles) : (size_t)(tiles - map);
    out_ok->tiles_strict_prefix = (tile_off + TITLE_LOGO_TILE_SIG_LEN <= rom_size
                                    && memcmp(tiles, sTitleLogoTilesLzSig, TITLE_LOGO_TILE_SIG_LEN) == 0)
        ? 1
        : 0;
    {
        size_t lo = (size_t)(pal - rom);
        size_t hi = lo;
        size_t t = (size_t)(tiles - rom);
        size_t u = (size_t)(map - rom);
        if (t < lo)
            lo = t;
        if (t > hi)
            hi = t;
        if (u < lo)
            lo = u;
        if (u > hi)
            hi = u;
        out_ok->cluster_span = hi - lo;
    }
    return TITLE_LOGO_TILE_TRY_OK;
}

static int title_logo_triple_better(const TitleLogoTriple *a, const TitleLogoTriple *b)
{
    if (a->tiles == NULL)
        return 1;
    if (b->tiles == NULL)
        return 0;
    if (b->map_from_after > a->map_from_after)
        return 1;
    if (b->map_from_after < a->map_from_after)
        return 0;
    if (b->map_kind == TITLE_LOGO_MAP_MATCH_STRICT_32 && a->map_kind != TITLE_LOGO_MAP_MATCH_STRICT_32)
        return 1;
    if (a->map_kind == TITLE_LOGO_MAP_MATCH_STRICT_32 && b->map_kind != TITLE_LOGO_MAP_MATCH_STRICT_32)
        return 0;
    if (b->tiles_strict_prefix > a->tiles_strict_prefix)
        return 1;
    if (b->tiles_strict_prefix < a->tiles_strict_prefix)
        return 0;
    if (b->map_distance != a->map_distance)
        return b->map_distance < a->map_distance;
    return b->cluster_span < a->cluster_span;
}

typedef struct TitleLogoBindCtx {
    const u8 **out_pal;
    const u8 **out_tiles;
    const u8 **out_map;
} TitleLogoBindCtx;

static void title_logo_clear_outputs(void *v)
{
    TitleLogoBindCtx *c = (TitleLogoBindCtx *)v;

    if (c == NULL)
        return;
    *c->out_pal = NULL;
    *c->out_tiles = NULL;
    *c->out_map = NULL;
}

static void title_logo_trace_rom_too_small(void *user, size_t rom_size, size_t min_sz, FireredRomAssetTrace *tr)
{
    (void)user;
    (void)min_sz;
    firered_portable_rom_asset_trace_printf(tr,
        "bind aborted: engine_memory_get_loaded_rom_size()=0x%zx (ROM not mapped / too small); fallback to *_Portable\n",
        rom_size);
}

static void title_logo_trace_rom_mapped(void *user, size_t rom_size, FireredRomAssetTrace *tr)
{
    (void)user;
    firered_portable_rom_asset_trace_printf(tr, "bind attempt: rom_size=0x%zx ENGINE_ROM_ADDR=0x%08x\n", rom_size,
        (unsigned)ENGINE_ROM_ADDR);
    firered_portable_rom_asset_trace_flush();
}

static void title_logo_trace_before_scan(void *user, size_t rom_size, FireredRomAssetTrace *tr)
{
    (void)user;
    (void)rom_size;
    firered_portable_rom_asset_trace_printf(tr, "env did not produce a full bind; trying signature scan\n");
    firered_portable_rom_asset_trace_flush();
}

static bool8 title_logo_try_env(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleLogoBindCtx *c = (TitleLogoBindCtx *)user;
    size_t pal_off = 0, tiles_off = 0, map_off = 0;
    int ok_pal, ok_tiles, ok_map;
    FireredRomAssetProfileOffsets prof;
    uint32_t need;

    ok_pal = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_LOGO_PAL_OFF", &pal_off);
    ok_tiles = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_LOGO_TILES_OFF", &tiles_off);
    ok_map = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_LOGO_MAP_OFF", &map_off);

    if (ok_pal && ok_tiles && ok_map)
    {
        if (pal_off + TITLE_LOGO_PAL_SIZE > rom_size || tiles_off + TITLE_LOGO_TILE_SIG_LEN > rom_size
            || map_off + TITLE_LOGO_MAP_SIG_LEN > rom_size)
        {
            char detail[192];
            size_t pos = 0;
            int nw;

            nw = snprintf(detail + pos, sizeof detail - pos, "env bind rejected (out of range vs rom_size=0x%zx): ",
                rom_size);
            if (nw > 0)
                pos += (size_t)nw;
            if (pal_off + TITLE_LOGO_PAL_SIZE > rom_size)
            {
                nw = snprintf(detail + pos, sizeof detail - pos, "pal_end=0x%zx ", pal_off + TITLE_LOGO_PAL_SIZE);
                if (nw > 0)
                    pos += (size_t)nw;
            }
            if (tiles_off + TITLE_LOGO_TILE_SIG_LEN > rom_size)
            {
                nw = snprintf(detail + pos, sizeof detail - pos, "tiles_need_end=0x%zx ",
                    tiles_off + TITLE_LOGO_TILE_SIG_LEN);
                if (nw > 0)
                    pos += (size_t)nw;
            }
            if (map_off + TITLE_LOGO_MAP_SIG_LEN > rom_size)
            {
                nw = snprintf(detail + pos, sizeof detail - pos, "map_need_end=0x%zx ", map_off + TITLE_LOGO_MAP_SIG_LEN);
                if (nw > 0)
                    pos += (size_t)nw;
            }
            if (pos + 1 < sizeof detail)
            {
                detail[pos++] = '\n';
                detail[pos] = '\0';
            }
            firered_portable_rom_asset_trace_printf(tr, "%s", detail);
            firered_portable_rom_asset_trace_flush();
            return FALSE;
        }

        *c->out_pal = rom + pal_off;
        *c->out_tiles = rom + tiles_off;
        *c->out_map = rom + map_off;

        firered_portable_rom_asset_trace_printf(tr, "env bind OK: pal=0x%zx tiles=0x%zx map=0x%zx\n", pal_off, tiles_off,
            map_off);
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }

    if (!ok_pal && !ok_tiles && !ok_map
        && firered_portable_rom_asset_profile_lookup(FIRERED_ROM_ASSET_FAMILY_TITLE_LOGO, &prof))
    {
        need = FIRERED_ROM_ASSET_PROFILE_HAS_PAL | FIRERED_ROM_ASSET_PROFILE_HAS_TILES | FIRERED_ROM_ASSET_PROFILE_HAS_MAP;
        if ((prof.set_mask & need) == need)
        {
            pal_off = prof.pal_off;
            tiles_off = prof.tiles_off;
            map_off = prof.map_off;
            if (pal_off + TITLE_LOGO_PAL_SIZE > rom_size || tiles_off + TITLE_LOGO_TILE_SIG_LEN > rom_size
                || map_off + TITLE_LOGO_MAP_SIG_LEN > rom_size)
            {
                firered_portable_rom_asset_trace_printf(tr,
                    "profile bind rejected (out of range vs rom_size=0x%zx)\n", rom_size);
                firered_portable_rom_asset_trace_flush();
                return FALSE;
            }

            *c->out_pal = rom + pal_off;
            *c->out_tiles = rom + tiles_off;
            *c->out_map = rom + map_off;

            firered_portable_rom_asset_trace_printf(tr,
                "profile bind OK: pal=0x%zx tiles=0x%zx map=0x%zx\n", pal_off, tiles_off, map_off);
            firered_portable_rom_asset_trace_flush();
            return TRUE;
        }
    }

    firered_portable_rom_asset_trace_printf(tr,
        "env/profile bind incomplete (need all three): PAL_OFF=%s TILES_OFF=%s MAP_OFF=%s\n",
        ok_pal ? "ok" : (firered_portable_rom_asset_env_nonempty("FIRERED_ROM_TITLE_LOGO_PAL_OFF") ? "invalid" : "unset"),
        ok_tiles ? "ok"
                 : (firered_portable_rom_asset_env_nonempty("FIRERED_ROM_TITLE_LOGO_TILES_OFF") ? "invalid" : "unset"),
        ok_map ? "ok" : (firered_portable_rom_asset_env_nonempty("FIRERED_ROM_TITLE_LOGO_MAP_OFF") ? "invalid" : "unset"));
    firered_portable_rom_asset_trace_flush();
    return FALSE;
}

static bool8 title_logo_try_scan(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleLogoBindCtx *c = (TitleLogoBindCtx *)user;
    size_t tile_match_offs[MAX_TILE_MATCHES];
    unsigned tile_match_count = 0u;
    size_t i;
    unsigned fail_lz = 0u, fail_pal_room = 0u, fail_map_layout = 0u;
    int saw_uncapped_tile_hits = 0;
    size_t ex_lz = (size_t)-1, ex_room = (size_t)-1, ex_map = (size_t)-1;

    if (rom_size < TITLE_LOGO_PAL_SIZE + TITLE_LOGO_TILE_SIG_LEN + TITLE_LOGO_MAP_SIG_LEN)
    {
        firered_portable_rom_asset_trace_printf(tr,
            "scan skipped: rom_size=0x%zx smaller than minimum for logo scan\n", rom_size);
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    title_rom_ensure_lz_indexes(rom, rom_size);

    for (i = 0; i + TITLE_LOGO_TILE_SIG_LEN <= rom_size; i += 4)
    {
        if (memcmp(rom + i, sTitleLogoTilesLzSig, TITLE_LOGO_TILE_SIG_LEN) != 0)
            continue;

        if (tile_match_count < MAX_TILE_MATCHES)
            tile_match_offs[tile_match_count++] = i;
        else
            saw_uncapped_tile_hits = 1;
    }

    firered_portable_rom_asset_trace_printf(tr, "scan: tile LZ signature hits=%u%s\n", tile_match_count,
        saw_uncapped_tile_hits ? " (capped; more in ROM)" : "");
    firered_portable_rom_asset_trace_flush();

    if (tile_match_count == 0u)
    {
        TitleLogoTriple best = {0};
        unsigned relaxed_triples = 0u;
        size_t j;

        firered_portable_rom_asset_trace_printf(tr,
            "scan: no strict tiles LZ prefix; trying relaxed tiles (LZ77 dec_size==0x%04x + parseable) + pal/map pairing\n",
            TITLE_LOGO_TILES_DECOMP_SIZE);
        firered_portable_rom_asset_trace_flush();

        for (j = 0; j < s_title_lz_count_dec0x4000; j++)
        {
            const u8 *tiles = rom + s_title_lz_off_dec0x4000[j];
            TitleLogoTriple t;
            int rc;

            rc = title_logo_try_tile_candidate(rom, rom_size, tiles, &t);
            if (rc != TITLE_LOGO_TILE_TRY_OK)
                continue;

            relaxed_triples++;
            if (best.tiles == NULL || title_logo_triple_better(&best, &t))
                best = t;
        }

        if (best.tiles != NULL)
        {
            *c->out_pal = best.pal;
            *c->out_tiles = best.tiles;
            *c->out_map = best.map;

            firered_portable_rom_asset_trace_printf(tr,
                "scan bind OK: pal=0x%zx tiles=0x%zx map=0x%zx (tiles_match=relaxed_dec0x%04x valid_triples=%u map_match=%s)\n",
                (size_t)(best.pal - rom), (size_t)(best.tiles - rom), (size_t)(best.map - rom),
                TITLE_LOGO_TILES_DECOMP_SIZE, relaxed_triples,
                best.map_kind == TITLE_LOGO_MAP_MATCH_STRICT_32 ? "strict_32b" : "lz_dec0x500");
            firered_portable_rom_asset_trace_flush();
            return TRUE;
        }

        firered_portable_rom_asset_trace_printf(tr,
            "scan: relaxed tiles pass found no pal+map triple (repacked or non-standard logo dimensions)\n");
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    for (i = 0; i < tile_match_count; i++)
    {
        const u8 *tiles = rom + tile_match_offs[i];
        size_t tiles_avail = rom_size - tile_match_offs[i];
        size_t tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, tiles_avail);
        TitleLogoTriple triple;
        int rc = title_logo_try_tile_candidate(rom, rom_size, tiles, &triple);

        if (rc == TITLE_LOGO_TILE_TRY_FAIL_LZ)
        {
            fail_lz++;
            if (ex_lz == (size_t)-1)
                ex_lz = tile_match_offs[i];
            continue;
        }
        if (rc == TITLE_LOGO_TILE_TRY_FAIL_PAL_ROOM)
        {
            fail_pal_room++;
            if (ex_room == (size_t)-1)
                ex_room = tile_match_offs[i];
            continue;
        }
        if (rc == TITLE_LOGO_TILE_TRY_FAIL_MAP)
        {
            fail_map_layout++;
            if (ex_map == (size_t)-1)
                ex_map = tile_match_offs[i];
            continue;
        }

        *c->out_pal = triple.pal;
        *c->out_tiles = triple.tiles;
        *c->out_map = triple.map;

        firered_portable_rom_asset_trace_printf(tr,
            "scan bind OK: pal=0x%zx tiles=0x%zx map=0x%zx (tiles_clen=0x%zx map_match=%s)\n",
            (size_t)(triple.pal - rom), (size_t)(triple.tiles - rom), (size_t)(triple.map - rom), tiles_clen,
            triple.map_kind == TITLE_LOGO_MAP_MATCH_STRICT_32 ? "strict_32b" : "lz_dec0x500");
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }

    {
        char aggregate[384];
        size_t pos = 0;
        int nw;

        nw = snprintf(aggregate + pos, sizeof aggregate - pos,
            "scan: no valid triple (lz_fail=%u pal_room_fail=%u map_layout_fail=%u)", fail_lz,
            fail_pal_room, fail_map_layout);
        if (nw > 0)
            pos += (size_t)nw;

        if (ex_lz != (size_t)-1 || ex_room != (size_t)-1 || ex_map != (size_t)-1)
        {
            nw = snprintf(aggregate + pos, sizeof aggregate - pos, " first_tile_off@");
            if (nw > 0)
                pos += (size_t)nw;
            if (ex_lz != (size_t)-1)
            {
                nw = snprintf(aggregate + pos, sizeof aggregate - pos, " lz=0x%zx", ex_lz);
                if (nw > 0)
                    pos += (size_t)nw;
            }
            if (ex_room != (size_t)-1)
            {
                nw = snprintf(aggregate + pos, sizeof aggregate - pos, " room=0x%zx", ex_room);
                if (nw > 0)
                    pos += (size_t)nw;
            }
            if (ex_map != (size_t)-1)
            {
                nw = snprintf(aggregate + pos, sizeof aggregate - pos, " map=0x%zx", ex_map);
                if (nw > 0)
                    pos += (size_t)nw;
            }
        }
        if (pos + 2 < sizeof aggregate)
        {
            aggregate[pos++] = '\n';
            aggregate[pos] = '\0';
        }
        firered_portable_rom_asset_trace_printf(tr, "%s", aggregate);
        if (fail_map_layout > 0u)
        {
            firered_portable_rom_asset_trace_printf(tr,
                "scan hint: after each tiles LZ end, searched 0x%zx bytes for map: "
                "32-byte stock prefix, else LZ77 dec_size==0x%03x\n",
                (size_t)TITLE_LOGO_MAP_SEARCH_MAX_AFTER_TILES, TITLE_LOGO_MAP_DECOMP_SIZE);
        }
        firered_portable_rom_asset_trace_flush();
    }

    return FALSE;
}

static const u8 *find_copyright_map_after_tiles(
    const u8 *rom, size_t rom_size, const u8 *tiles, size_t tiles_compressed_len, int *map_match_kind_out)
{
    const u8 *region_start = tiles + tiles_compressed_len;
    const u8 *rom_end = rom + rom_size;
    FireredRomLzFollowupKind k = FIRERED_ROM_LZ_FOLLOWUP_NONE;
    const u8 *map;

    if (map_match_kind_out != NULL)
        *map_match_kind_out = COPYRIGHT_MAP_MATCH_NONE;

    if (region_start > rom_end)
        return NULL;

    map = firered_portable_rom_find_lz_followup(region_start, (size_t)(rom_end - region_start),
                                                COPYRIGHT_MAP_SEARCH_MAX_AFTER_TILES, sCopyrightMapLzSig,
                                                COPYRIGHT_MAP_SIG_LEN, COPYRIGHT_MAP_DECOMP_SIZE, &k);

    if (map_match_kind_out != NULL)
    {
        if (k == FIRERED_ROM_LZ_FOLLOWUP_STRICT_SIG)
            *map_match_kind_out = COPYRIGHT_MAP_MATCH_STRICT_32;
        else if (k == FIRERED_ROM_LZ_FOLLOWUP_RELAXED_DEC)
            *map_match_kind_out = COPYRIGHT_MAP_MATCH_LZ_DEC0x500;
    }

    return map;
}

/*
 * When the tilemap LZ is not adjacent after the tiles blob (common when hacks
 * pad/reorder rodata), pick the best parseable copyright map LZ with retail
 * decompressed size 0x500 — same ranking idea as game_title_logo.
 */
static const u8 *find_copyright_map_anywhere_for_tiles(
    const u8 *rom, size_t rom_size, const u8 *tiles, int *map_match_kind_out)
{
    size_t j;
    const u8 *best = NULL;
    size_t best_distance = (size_t)-1;
    int best_kind = COPYRIGHT_MAP_MATCH_NONE;
    int best_after = 0;

    title_rom_ensure_lz_indexes(rom, rom_size);

    for (j = 0; j < s_title_lz_count_dec0x500; j++)
    {
        size_t i = s_title_lz_off_dec0x500[j];
        const u8 *candidate = rom + i;
        int kind;
        int after;
        size_t distance;

        kind = (i + COPYRIGHT_MAP_SIG_LEN <= rom_size
                && memcmp(candidate, sCopyrightMapLzSig, COPYRIGHT_MAP_SIG_LEN) == 0)
            ? COPYRIGHT_MAP_MATCH_STRICT_32
            : COPYRIGHT_MAP_MATCH_LZ_DEC0x500;
        after = (candidate >= tiles);
        distance = after ? (size_t)(candidate - tiles) : (size_t)(tiles - candidate);

        if (best == NULL
            || (kind == COPYRIGHT_MAP_MATCH_STRICT_32 && best_kind != COPYRIGHT_MAP_MATCH_STRICT_32)
            || (kind == best_kind && after && !best_after)
            || (kind == best_kind && after == best_after && distance < best_distance))
        {
            best = candidate;
            best_distance = distance;
            best_kind = kind;
            best_after = after;
        }
    }

    if (map_match_kind_out != NULL)
        *map_match_kind_out = best_kind;
    return best;
}

typedef struct CopyrightTilePair {
    const u8 *tiles;
    const u8 *map;
    int map_kind;
    int map_from_after;
    size_t map_distance;
    int tiles_strict_prefix;
} CopyrightTilePair;

static int title_copyright_try_tile_pair(
    const u8 *rom, size_t rom_size, const u8 *tiles, CopyrightTilePair *out_ok)
{
    size_t tile_off = (size_t)(tiles - rom);
    size_t tiles_avail = rom_size - tile_off;
    size_t tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, tiles_avail);
    const u8 *map;
    int mk = COPYRIGHT_MAP_MATCH_NONE;

    if (tiles_clen == 0)
        return COPYRIGHT_TILE_TRY_FAIL_LZ;

    map = find_copyright_map_after_tiles(rom, rom_size, tiles, tiles_clen, &mk);
    out_ok->map_from_after = (map != NULL);
    if (map == NULL)
    {
        map = find_copyright_map_anywhere_for_tiles(rom, rom_size, tiles, &mk);
        if (map == NULL)
            return COPYRIGHT_TILE_TRY_FAIL_MAP;
    }

    out_ok->tiles = tiles;
    out_ok->map = map;
    out_ok->map_kind = mk;
    out_ok->map_distance = (map >= tiles) ? (size_t)(map - tiles) : (size_t)(tiles - map);
    out_ok->tiles_strict_prefix = (tile_off + COPYRIGHT_TILES_SIG_LEN <= rom_size
                                   && memcmp(tiles, sCopyrightTilesLzSig, COPYRIGHT_TILES_SIG_LEN) == 0)
        ? 1
        : 0;
    return COPYRIGHT_TILE_TRY_OK;
}

static int title_copyright_pair_better(const CopyrightTilePair *a, const CopyrightTilePair *b)
{
    if (a->tiles == NULL)
        return 1;
    if (b->tiles == NULL)
        return 0;
    if (b->map_from_after > a->map_from_after)
        return 1;
    if (b->map_from_after < a->map_from_after)
        return 0;
    if (b->map_kind == COPYRIGHT_MAP_MATCH_STRICT_32 && a->map_kind != COPYRIGHT_MAP_MATCH_STRICT_32)
        return 1;
    if (a->map_kind == COPYRIGHT_MAP_MATCH_STRICT_32 && b->map_kind != COPYRIGHT_MAP_MATCH_STRICT_32)
        return 0;
    if (b->tiles_strict_prefix > a->tiles_strict_prefix)
        return 1;
    if (b->tiles_strict_prefix < a->tiles_strict_prefix)
        return 0;
    return b->map_distance < a->map_distance;
}

static const u8 *find_box_art_map_after_tiles(
    const u8 *rom, size_t rom_size, const u8 *tiles, size_t tiles_compressed_len)
{
    const u8 *region_start = tiles + tiles_compressed_len;
    const u8 *rom_end = rom + rom_size;

    if (region_start > rom_end)
        return NULL;

    return firered_portable_rom_find_lz_followup(region_start, (size_t)(rom_end - region_start),
                                                 BOX_ART_MAP_SEARCH_MAX_AFTER_TILES, sBoxArtMapLzSig,
                                                 BOX_ART_MAP_SIG_LEN, BOX_ART_MAP_DECOMP_SIZE, NULL);
}

static const u8 *find_border_bg_map_after_tiles(
    const u8 *rom, size_t rom_size, const u8 *tiles, size_t tiles_compressed_len)
{
    const u8 *region_start = tiles + tiles_compressed_len;
    const u8 *rom_end = rom + rom_size;

    if (region_start > rom_end)
        return NULL;

    return firered_portable_rom_find_lz_followup(region_start, (size_t)(rom_end - region_start),
                                                 BORDER_BG_MAP_SEARCH_MAX_AFTER_TILES, sBorderBgMapLzSig,
                                                 BORDER_BG_MAP_SIG_LEN, BORDER_BG_MAP_DECOMP_SIZE, NULL);
}

static int title_logo_baseline_candidate_present(const u8 *rom, size_t rom_size)
{
    size_t i;

    for (i = 0; i + TITLE_LOGO_TILE_SIG_LEN <= rom_size; i += 4)
    {
        const u8 *tiles;
        size_t tiles_clen;
        const u8 *pal;
        const u8 *map;

        if (memcmp(rom + i, sTitleLogoTilesLzSig, TITLE_LOGO_TILE_SIG_LEN) != 0)
            continue;
        tiles = rom + i;
        tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, rom_size - i);
        if (tiles_clen == 0 || i < TITLE_LOGO_PAL_SIZE)
            continue;
        map = find_map_after_tiles(rom, rom_size, tiles, tiles_clen, NULL);
        if (map == NULL)
            continue;
        pal = tiles - TITLE_LOGO_PAL_SIZE;
        if (title_logo_bound_equals_baseline(rom, rom_size, pal, tiles, map))
            return 1;
    }

    return 0;
}

static int title_copyright_baseline_candidate_present(const u8 *rom, size_t rom_size)
{
    size_t i;

    for (i = 0; i + COPYRIGHT_TILES_SIG_LEN <= rom_size; i += 4)
    {
        const u8 *tiles;
        size_t tiles_clen;
        const u8 *map;

        if (memcmp(rom + i, sCopyrightTilesLzSig, COPYRIGHT_TILES_SIG_LEN) != 0)
            continue;
        tiles = rom + i;
        tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, rom_size - i);
        if (tiles_clen == 0)
            continue;
        map = find_copyright_map_after_tiles(rom, rom_size, tiles, tiles_clen, NULL);
        if (map == NULL)
            map = find_copyright_map_anywhere_for_tiles(rom, rom_size, tiles, NULL);
        if (map == NULL)
            continue;
        if (title_copyright_bound_equals_baseline(rom, rom_size, tiles, map))
            return 1;
    }

    return 0;
}

typedef struct TitleCopyrightBindCtx {
    const u8 **out_tiles;
    const u8 **out_map;
} TitleCopyrightBindCtx;

static void title_copyright_clear_outputs(void *v)
{
    TitleCopyrightBindCtx *c = (TitleCopyrightBindCtx *)v;

    if (c == NULL)
        return;
    *c->out_tiles = NULL;
    *c->out_map = NULL;
}

static void title_copyright_trace_scan_begin(void *user, size_t rom_size, FireredRomAssetTrace *tr)
{
    (void)user;
    firered_portable_rom_asset_trace_printf(tr, "bind attempt: rom_size=0x%zx scan_enabled=1\n", rom_size);
    firered_portable_rom_asset_trace_flush();
}

static bool8 title_copyright_try_env(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleCopyrightBindCtx *c = (TitleCopyrightBindCtx *)user;
    size_t tiles_off = 0, map_off = 0;
    int ok_tiles, ok_map;
    FireredRomAssetProfileOffsets prof;
    uint32_t need;

    ok_tiles = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_COPYRIGHT_TILES_OFF", &tiles_off);
    ok_map = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_COPYRIGHT_MAP_OFF", &map_off);

    if (ok_tiles && ok_map)
    {
        if (tiles_off + COPYRIGHT_TILES_SIG_LEN > rom_size || map_off + COPYRIGHT_MAP_SIG_LEN > rom_size)
        {
            firered_portable_rom_asset_trace_printf(tr, "env bind rejected (out of range vs rom_size=0x%zx)\n", rom_size);
            firered_portable_rom_asset_trace_flush();
            return FALSE;
        }

        *c->out_tiles = rom + tiles_off;
        *c->out_map = rom + map_off;

        firered_portable_rom_asset_trace_printf(tr, "env bind OK: tiles=0x%zx map=0x%zx\n", tiles_off, map_off);
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }

    if (!ok_tiles && !ok_map
        && firered_portable_rom_asset_profile_lookup(FIRERED_ROM_ASSET_FAMILY_COPYRIGHT_PRESS_START, &prof))
    {
        need = FIRERED_ROM_ASSET_PROFILE_HAS_TILES | FIRERED_ROM_ASSET_PROFILE_HAS_MAP;
        if ((prof.set_mask & need) == need)
        {
            tiles_off = prof.tiles_off;
            map_off = prof.map_off;
            if (tiles_off + COPYRIGHT_TILES_SIG_LEN > rom_size || map_off + COPYRIGHT_MAP_SIG_LEN > rom_size)
            {
                firered_portable_rom_asset_trace_printf(tr,
                    "profile bind rejected (out of range vs rom_size=0x%zx)\n", rom_size);
                firered_portable_rom_asset_trace_flush();
                return FALSE;
            }

            *c->out_tiles = rom + tiles_off;
            *c->out_map = rom + map_off;

            firered_portable_rom_asset_trace_printf(tr, "profile bind OK: tiles=0x%zx map=0x%zx\n", tiles_off, map_off);
            firered_portable_rom_asset_trace_flush();
            return TRUE;
        }
    }

    firered_portable_rom_asset_trace_printf(tr, "env/profile bind incomplete: TILES_OFF=%s MAP_OFF=%s\n",
        ok_tiles ? "ok" : "missing/invalid", ok_map ? "ok" : "missing/invalid");
    firered_portable_rom_asset_trace_flush();
    return FALSE;
}

static bool8 title_copyright_try_scan(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleCopyrightBindCtx *c = (TitleCopyrightBindCtx *)user;
    size_t tile_match_offs[MAX_COPYRIGHT_TILE_MATCHES];
    unsigned tile_match_count = 0u;
    size_t i;
    unsigned fail_lz = 0u, fail_map = 0u;
    int saw_uncapped = 0;

    if (rom_size < COPYRIGHT_TILES_SIG_LEN + COPYRIGHT_MAP_SIG_LEN)
    {
        firered_portable_rom_asset_trace_printf(tr, "scan skipped: rom too small\n");
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    title_rom_ensure_lz_indexes(rom, rom_size);

    for (i = 0; i + COPYRIGHT_TILES_SIG_LEN <= rom_size; i += 4)
    {
        if (memcmp(rom + i, sCopyrightTilesLzSig, COPYRIGHT_TILES_SIG_LEN) != 0)
            continue;

        if (tile_match_count < MAX_COPYRIGHT_TILE_MATCHES)
            tile_match_offs[tile_match_count++] = i;
        else
            saw_uncapped = 1;
    }

    firered_portable_rom_asset_trace_printf(tr, "scan: tile LZ hits=%u%s\n", tile_match_count,
        saw_uncapped ? " (capped)" : "");
    firered_portable_rom_asset_trace_flush();

    if (tile_match_count == 0u)
    {
        CopyrightTilePair best = {0};
        unsigned valid_pairs = 0u;
        size_t j;

        firered_portable_rom_asset_trace_printf(tr,
            "scan: no strict tiles LZ prefix; trying relaxed tiles (LZ77 dec_size==0x%04x + parseable) + map pairing\n",
            COPYRIGHT_TILES_DECOMP_SIZE);
        firered_portable_rom_asset_trace_flush();

        for (j = 0; j < s_title_lz_count_dec0x800; j++)
        {
            const u8 *tiles = rom + s_title_lz_off_dec0x800[j];
            CopyrightTilePair t;
            int rc;

            rc = title_copyright_try_tile_pair(rom, rom_size, tiles, &t);
            if (rc != COPYRIGHT_TILE_TRY_OK)
                continue;

            valid_pairs++;
            if (best.tiles == NULL || title_copyright_pair_better(&best, &t))
                best = t;
        }

        if (best.tiles != NULL)
        {
            *c->out_tiles = best.tiles;
            *c->out_map = best.map;

            firered_portable_rom_asset_trace_printf(tr,
                "scan bind OK: tiles=0x%zx map=0x%zx (tiles_match=relaxed_dec0x%04x valid_pairs=%u map_match=%s)\n",
                (size_t)(best.tiles - rom), (size_t)(best.map - rom), COPYRIGHT_TILES_DECOMP_SIZE, valid_pairs,
                best.map_kind == COPYRIGHT_MAP_MATCH_STRICT_32 ? "strict_32b" : "lz_dec0x500");
            firered_portable_rom_asset_trace_flush();
            return TRUE;
        }

        firered_portable_rom_asset_trace_printf(tr,
            "scan: relaxed tiles pass found no tiles+map pair (repacked or non-standard strip size)\n");
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    for (i = 0; i < tile_match_count; i++)
    {
        const u8 *tiles = rom + tile_match_offs[i];
        size_t tiles_avail = rom_size - tile_match_offs[i];
        size_t tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, tiles_avail);
        CopyrightTilePair pair;
        int rc = title_copyright_try_tile_pair(rom, rom_size, tiles, &pair);

        if (rc == COPYRIGHT_TILE_TRY_FAIL_LZ)
        {
            fail_lz++;
            continue;
        }
        if (rc == COPYRIGHT_TILE_TRY_FAIL_MAP)
        {
            fail_map++;
            continue;
        }

        *c->out_tiles = pair.tiles;
        *c->out_map = pair.map;

        firered_portable_rom_asset_trace_printf(tr,
            "scan bind OK: tiles=0x%zx map=0x%zx tiles_clen=0x%zx map_match=%s\n", (size_t)(pair.tiles - rom),
            (size_t)(pair.map - rom), tiles_clen,
            pair.map_kind == COPYRIGHT_MAP_MATCH_STRICT_32 ? "strict_32b" : "lz_dec0x500");
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }

    firered_portable_rom_asset_trace_printf(tr, "scan: no valid pair (lz_fail=%u map_fail=%u)\n", fail_lz, fail_map);
    firered_portable_rom_asset_trace_flush();
    return FALSE;
}

void firered_portable_title_screen_try_bind_game_title_logo(
    const u8 **out_pal, const u8 **out_tiles, const u8 **out_map)
{
    TitleLogoBindCtx ctx;
    FireredRomAssetBindParams params;
    const u8 *rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    size_t rom_size = engine_memory_get_loaded_rom_size();
    bool8 bound;

    *out_pal = NULL;
    *out_tiles = NULL;
    *out_map = NULL;

    ctx.out_pal = out_pal;
    ctx.out_tiles = out_tiles;
    ctx.out_map = out_map;

    memset(&params, 0, sizeof(params));
    params.trace_env_var = "FIRERED_TRACE_TITLE_ROM";
    params.trace_prefix = "[firered title-rom]";
    params.min_rom_size = 0x200u;
    params.user = &ctx;
    params.clear_outputs = title_logo_clear_outputs;
    params.try_env = title_logo_try_env;
    params.try_scan = title_logo_try_scan;
    params.trace_rom_mapped = title_logo_trace_rom_mapped;
    params.trace_rom_too_small = title_logo_trace_rom_too_small;
    params.trace_scan_beginning = title_logo_trace_before_scan;
    params.trace_fallback_line = "fallback: using compiled *_Portable game title logo (not ROM-backed for this load)\n";
    params.use_compat_scan_gate = FALSE;
    params.force_scan_env_var = NULL;

    bound = firered_portable_rom_asset_bind_run(&params);

    if (bound)
    {
        if (title_logo_bound_equals_baseline(rom, rom_size, *out_pal, *out_tiles, *out_map))
            s_title_core_provenance_game_title_logo = FIRERED_TITLE_CORE_PROVENANCE_UNCHANGED;
        else
            s_title_core_provenance_game_title_logo = FIRERED_TITLE_CORE_PROVENANCE_CHANGED_AND_BOUND;
        return;
    }

    if (title_logo_env_override_present())
    {
        s_title_core_provenance_game_title_logo = FIRERED_TITLE_CORE_PROVENANCE_CHANGED_BUT_FELL_BACK;
        return;
    }

    s_title_core_provenance_game_title_logo = title_logo_baseline_candidate_present(rom, rom_size)
        ? FIRERED_TITLE_CORE_PROVENANCE_UNCHANGED
        : FIRERED_TITLE_CORE_PROVENANCE_CHANGED_BUT_FELL_BACK;
}

void firered_portable_title_screen_try_bind_copyright_press_start(const u8 **out_tiles, const u8 **out_map)
{
    TitleCopyrightBindCtx ctx;
    FireredRomAssetBindParams params;
    const u8 *rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    size_t rom_size = engine_memory_get_loaded_rom_size();
    bool8 bound;

    *out_tiles = NULL;
    *out_map = NULL;

    ctx.out_tiles = out_tiles;
    ctx.out_map = out_map;

    memset(&params, 0, sizeof(params));
    params.trace_env_var = "FIRERED_TRACE_TITLE_ROM";
    params.trace_prefix = "[firered title-rom copyright]";
    params.min_rom_size = 0x200u;
    params.user = &ctx;
    params.clear_outputs = title_copyright_clear_outputs;
    params.try_env = title_copyright_try_env;
    params.try_scan = title_copyright_try_scan;
    params.trace_scan_beginning = title_copyright_trace_scan_begin;
    params.trace_fallback_line = "fallback: compiled *_Portable copyright/press-start graphics\n";
    params.use_compat_scan_gate = FALSE;
    params.force_scan_env_var = NULL;

    bound = firered_portable_rom_asset_bind_run(&params);

    if (bound)
    {
        if (title_copyright_bound_equals_baseline(rom, rom_size, *out_tiles, *out_map))
            s_title_core_provenance_copyright_press_start = FIRERED_TITLE_CORE_PROVENANCE_UNCHANGED;
        else
            s_title_core_provenance_copyright_press_start = FIRERED_TITLE_CORE_PROVENANCE_CHANGED_AND_BOUND;
        return;
    }

    if (title_copyright_env_override_present())
    {
        s_title_core_provenance_copyright_press_start = FIRERED_TITLE_CORE_PROVENANCE_CHANGED_BUT_FELL_BACK;
        return;
    }

    s_title_core_provenance_copyright_press_start = title_copyright_baseline_candidate_present(rom, rom_size)
        ? FIRERED_TITLE_CORE_PROVENANCE_UNCHANGED
        : FIRERED_TITLE_CORE_PROVENANCE_CHANGED_BUT_FELL_BACK;
}

typedef struct TitleBoxArtBindCtx {
    const u8 **out_pal;
    const u8 **out_tiles;
    const u8 **out_map;
} TitleBoxArtBindCtx;

typedef struct TitleBorderBgBindCtx {
    const u8 **out_tiles;
    const u8 **out_map;
} TitleBorderBgBindCtx;

typedef struct TitleBackgroundPalsBindCtx {
    const u8 **out_pal;
} TitleBackgroundPalsBindCtx;

static void title_box_art_clear_outputs(void *v)
{
    TitleBoxArtBindCtx *c = (TitleBoxArtBindCtx *)v;
    if (c == NULL)
        return;
    *c->out_pal = NULL;
    *c->out_tiles = NULL;
    *c->out_map = NULL;
}

static bool8 title_box_art_try_env(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleBoxArtBindCtx *c = (TitleBoxArtBindCtx *)user;
    size_t pal_off = 0, tiles_off = 0, map_off = 0;
    int ok_pal = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_BOX_ART_PAL_OFF", &pal_off);
    int ok_tiles = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_BOX_ART_TILES_OFF", &tiles_off);
    int ok_map = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_BOX_ART_MAP_OFF", &map_off);

    if (!ok_pal || !ok_tiles || !ok_map)
        return FALSE;
    if (pal_off + BOX_ART_PAL_SIZE > rom_size || tiles_off + BOX_ART_TILES_SIG_LEN > rom_size
        || map_off + BOX_ART_MAP_SIG_LEN > rom_size)
        return FALSE;

    *c->out_pal = rom + pal_off;
    *c->out_tiles = rom + tiles_off;
    *c->out_map = rom + map_off;
    firered_portable_rom_asset_trace_printf(tr, "env bind OK: pal=0x%zx tiles=0x%zx map=0x%zx\n", pal_off, tiles_off, map_off);
    firered_portable_rom_asset_trace_flush();
    return TRUE;
}

static bool8 title_box_art_try_scan(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleBoxArtBindCtx *c = (TitleBoxArtBindCtx *)user;
    size_t tile_match_offs[MAX_BOX_ART_TILE_MATCHES];
    unsigned tile_match_count = 0u;
    size_t i;

    for (i = 0; i + BOX_ART_TILES_SIG_LEN <= rom_size; i += 4)
    {
        if (memcmp(rom + i, sBoxArtTilesLzSig, BOX_ART_TILES_SIG_LEN) != 0)
            continue;
        if (tile_match_count < MAX_BOX_ART_TILE_MATCHES)
            tile_match_offs[tile_match_count++] = i;
    }
    firered_portable_rom_asset_trace_printf(tr, "scan: box_art tile hits=%u\n", tile_match_count);
    firered_portable_rom_asset_trace_flush();
    for (i = 0; i < tile_match_count; i++)
    {
        const u8 *tiles = rom + tile_match_offs[i];
        size_t tiles_clen;
        const u8 *pal;
        const u8 *map;
        if (tile_match_offs[i] < BOX_ART_PAL_SIZE)
            continue;
        tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, rom_size - tile_match_offs[i]);
        if (tiles_clen == 0)
            continue;
        pal = tiles - BOX_ART_PAL_SIZE;
        map = find_box_art_map_after_tiles(rom, rom_size, tiles, tiles_clen);
        if (map == NULL)
            continue;
        *c->out_pal = pal;
        *c->out_tiles = tiles;
        *c->out_map = map;
        firered_portable_rom_asset_trace_printf(tr, "scan bind OK: pal=0x%zx tiles=0x%zx map=0x%zx\n",
                                                (size_t)(pal - rom), (size_t)(tiles - rom), (size_t)(map - rom));
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }
    return FALSE;
}

void firered_portable_title_screen_try_bind_box_art_mon(
    const u8 **out_pal, const u8 **out_tiles, const u8 **out_map)
{
    TitleBoxArtBindCtx ctx;
    FireredRomAssetBindParams params;
    *out_pal = NULL;
    *out_tiles = NULL;
    *out_map = NULL;
    ctx.out_pal = out_pal;
    ctx.out_tiles = out_tiles;
    ctx.out_map = out_map;
    memset(&params, 0, sizeof(params));
    params.trace_env_var = "FIRERED_TRACE_TITLE_ROM";
    params.trace_prefix = "[firered title-rom box-art]";
    params.min_rom_size = 0x200u;
    params.user = &ctx;
    params.clear_outputs = title_box_art_clear_outputs;
    params.try_env = title_box_art_try_env;
    params.try_scan = title_box_art_try_scan;
    params.trace_fallback_line = "fallback: compiled *_Portable box-art title assets\n";
    params.use_compat_scan_gate = FALSE;
    params.force_scan_env_var = NULL;
    firered_portable_rom_asset_bind_run(&params);
}

static void title_border_bg_clear_outputs(void *v)
{
    TitleBorderBgBindCtx *c = (TitleBorderBgBindCtx *)v;
    if (c == NULL)
        return;
    *c->out_tiles = NULL;
    *c->out_map = NULL;
}

static bool8 title_border_bg_try_env(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleBorderBgBindCtx *c = (TitleBorderBgBindCtx *)user;
    size_t tiles_off = 0, map_off = 0;
    int ok_tiles = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_BORDER_TILES_OFF", &tiles_off);
    int ok_map = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_BORDER_MAP_OFF", &map_off);
    if (!ok_tiles || !ok_map)
        return FALSE;
    if (tiles_off + BORDER_BG_TILES_SIG_LEN > rom_size || map_off + BORDER_BG_MAP_SIG_LEN > rom_size)
        return FALSE;
    *c->out_tiles = rom + tiles_off;
    *c->out_map = rom + map_off;
    firered_portable_rom_asset_trace_printf(tr, "env bind OK: tiles=0x%zx map=0x%zx\n", tiles_off, map_off);
    firered_portable_rom_asset_trace_flush();
    return TRUE;
}

static bool8 title_border_bg_try_scan(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleBorderBgBindCtx *c = (TitleBorderBgBindCtx *)user;
    size_t tile_match_offs[MAX_BORDER_BG_TILE_MATCHES];
    unsigned tile_match_count = 0u;
    size_t i;
    for (i = 0; i + BORDER_BG_TILES_SIG_LEN <= rom_size; i += 4)
    {
        if (memcmp(rom + i, sBorderBgTilesLzSig, BORDER_BG_TILES_SIG_LEN) != 0)
            continue;
        if (tile_match_count < MAX_BORDER_BG_TILE_MATCHES)
            tile_match_offs[tile_match_count++] = i;
    }
    firered_portable_rom_asset_trace_printf(tr, "scan: border_bg tile hits=%u\n", tile_match_count);
    firered_portable_rom_asset_trace_flush();
    for (i = 0; i < tile_match_count; i++)
    {
        const u8 *tiles = rom + tile_match_offs[i];
        const u8 *map;
        size_t tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, rom_size - tile_match_offs[i]);
        if (tiles_clen == 0)
            continue;
        map = find_border_bg_map_after_tiles(rom, rom_size, tiles, tiles_clen);
        if (map == NULL)
            continue;
        *c->out_tiles = tiles;
        *c->out_map = map;
        firered_portable_rom_asset_trace_printf(tr, "scan bind OK: tiles=0x%zx map=0x%zx\n",
                                                (size_t)(tiles - rom), (size_t)(map - rom));
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }
    return FALSE;
}

void firered_portable_title_screen_try_bind_border_bg(const u8 **out_tiles, const u8 **out_map)
{
    TitleBorderBgBindCtx ctx;
    FireredRomAssetBindParams params;
    *out_tiles = NULL;
    *out_map = NULL;
    ctx.out_tiles = out_tiles;
    ctx.out_map = out_map;
    memset(&params, 0, sizeof(params));
    params.trace_env_var = "FIRERED_TRACE_TITLE_ROM";
    params.trace_prefix = "[firered title-rom border]";
    params.min_rom_size = 0x200u;
    params.user = &ctx;
    params.clear_outputs = title_border_bg_clear_outputs;
    params.try_env = title_border_bg_try_env;
    params.try_scan = title_border_bg_try_scan;
    params.trace_fallback_line = "fallback: compiled *_Portable border BG assets\n";
    params.use_compat_scan_gate = FALSE;
    params.force_scan_env_var = NULL;
    firered_portable_rom_asset_bind_run(&params);
}

static void title_background_pals_clear_outputs(void *v)
{
    TitleBackgroundPalsBindCtx *c = (TitleBackgroundPalsBindCtx *)v;
    if (c == NULL)
        return;
    *c->out_pal = NULL;
}

static bool8 title_background_pals_try_env(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleBackgroundPalsBindCtx *c = (TitleBackgroundPalsBindCtx *)user;
    size_t pal_off = 0;
    if (!firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_BG_PALS_OFF", &pal_off))
        return FALSE;
    if (pal_off + sizeof(gGraphics_TitleScreen_BackgroundPals_Portable) > rom_size)
        return FALSE;
    *c->out_pal = rom + pal_off;
    firered_portable_rom_asset_trace_printf(tr, "env bind OK: bg_pals=0x%zx\n", pal_off);
    firered_portable_rom_asset_trace_flush();
    return TRUE;
}

static bool8 title_background_pals_try_scan(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleBackgroundPalsBindCtx *c = (TitleBackgroundPalsBindCtx *)user;
    size_t i;
    size_t pal_size = sizeof(gGraphics_TitleScreen_BackgroundPals_Portable);
    for (i = 0; i + pal_size <= rom_size; i += 2)
    {
        if (memcmp(rom + i, gGraphics_TitleScreen_BackgroundPals_Portable, pal_size) == 0)
        {
            *c->out_pal = rom + i;
            firered_portable_rom_asset_trace_printf(tr, "scan bind OK: bg_pals=0x%zx\n", i);
            firered_portable_rom_asset_trace_flush();
            return TRUE;
        }
    }
    return FALSE;
}

void firered_portable_title_screen_try_bind_background_pals(const u8 **out_pal)
{
    TitleBackgroundPalsBindCtx ctx;
    FireredRomAssetBindParams params;
    *out_pal = NULL;
    ctx.out_pal = out_pal;
    memset(&params, 0, sizeof(params));
    params.trace_env_var = "FIRERED_TRACE_TITLE_ROM";
    params.trace_prefix = "[firered title-rom bg-pals]";
    params.min_rom_size = 0x200u;
    params.user = &ctx;
    params.clear_outputs = title_background_pals_clear_outputs;
    params.try_env = title_background_pals_try_env;
    params.try_scan = title_background_pals_try_scan;
    params.trace_fallback_line = "fallback: compiled *_Portable background palette\n";
    params.use_compat_scan_gate = FALSE;
    params.force_scan_env_var = NULL;
    firered_portable_rom_asset_bind_run(&params);
}

typedef struct TitleFlameFxBindCtx {
    const u16 **out_flames_pal;
    const u32 **out_flames_gfx;
    const u32 **out_blank_flames_gfx;
} TitleFlameFxBindCtx;

typedef struct TitleSlashBindCtx {
    const u32 **out_slash_gfx;
} TitleSlashBindCtx;

static void title_flame_fx_clear_outputs(void *v)
{
    TitleFlameFxBindCtx *c = (TitleFlameFxBindCtx *)v;
    if (c == NULL)
        return;
    *c->out_flames_pal = NULL;
    *c->out_flames_gfx = NULL;
    *c->out_blank_flames_gfx = NULL;
}

static bool8 title_flame_fx_try_env(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleFlameFxBindCtx *c = (TitleFlameFxBindCtx *)user;
    size_t pal_off = 0, flames_off = 0, blank_off = 0;
    int ok_pal = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_FLAMES_PAL_OFF", &pal_off);
    int ok_flames = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_FLAMES_GFX_OFF", &flames_off);
    int ok_blank = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_BLANK_FLAMES_GFX_OFF", &blank_off);
    size_t flames_clen, blank_clen;

    if (!ok_pal || !ok_flames || !ok_blank)
        return FALSE;
    if (pal_off + FLAMES_PAL_SIZE > rom_size || flames_off + TITLE_EFFECT_SIG_LEN > rom_size
        || blank_off + TITLE_EFFECT_SIG_LEN > rom_size)
        return FALSE;
    flames_clen = firered_portable_rom_lz77_compressed_size(rom + flames_off, rom_size - flames_off);
    blank_clen = firered_portable_rom_lz77_compressed_size(rom + blank_off, rom_size - blank_off);
    if (flames_clen == 0 || blank_clen == 0)
        return FALSE;

    *c->out_flames_pal = (const u16 *)(rom + pal_off);
    *c->out_flames_gfx = (const u32 *)(rom + flames_off);
    *c->out_blank_flames_gfx = (const u32 *)(rom + blank_off);
    firered_portable_rom_asset_trace_printf(tr, "env bind OK: flames_pal=0x%zx flames_gfx=0x%zx blank_flames_gfx=0x%zx\n",
                                            pal_off, flames_off, blank_off);
    firered_portable_rom_asset_trace_flush();
    return TRUE;
}

static bool8 title_flame_fx_try_scan(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleFlameFxBindCtx *c = (TitleFlameFxBindCtx *)user;
    const u8 *pal = NULL;
    const u8 *flames = find_lz_by_signature(rom, rom_size, sFlamesGfxLzSig, TITLE_EFFECT_SIG_LEN);
    const u8 *blank = find_lz_by_signature(rom, rom_size, sBlankFlamesGfxLzSig, TITLE_EFFECT_SIG_LEN);
    size_t i;

    if (flames == NULL || blank == NULL)
        return FALSE;
    for (i = 0; i + FLAMES_PAL_SIZE <= rom_size; i += 2)
    {
        if (memcmp(rom + i, sFlames_Pal_Portable, FLAMES_PAL_SIZE) == 0)
        {
            pal = rom + i;
            break;
        }
    }
    if (pal == NULL)
        return FALSE;

    *c->out_flames_pal = (const u16 *)pal;
    *c->out_flames_gfx = (const u32 *)flames;
    *c->out_blank_flames_gfx = (const u32 *)blank;
    firered_portable_rom_asset_trace_printf(tr, "scan bind OK: flames_pal=0x%zx flames_gfx=0x%zx blank_flames_gfx=0x%zx\n",
                                            (size_t)(pal - rom), (size_t)(flames - rom), (size_t)(blank - rom));
    firered_portable_rom_asset_trace_flush();
    return TRUE;
}

void firered_portable_title_screen_try_bind_flame_effect_assets(
    const u16 **out_flames_pal, const u32 **out_flames_gfx, const u32 **out_blank_flames_gfx)
{
    TitleFlameFxBindCtx ctx;
    FireredRomAssetBindParams params;

    *out_flames_pal = NULL;
    *out_flames_gfx = NULL;
    *out_blank_flames_gfx = NULL;
    ctx.out_flames_pal = out_flames_pal;
    ctx.out_flames_gfx = out_flames_gfx;
    ctx.out_blank_flames_gfx = out_blank_flames_gfx;
    memset(&params, 0, sizeof(params));
    params.trace_env_var = "FIRERED_TRACE_TITLE_ROM";
    params.trace_prefix = "[firered title-rom flames]";
    params.min_rom_size = 0x200u;
    params.user = &ctx;
    params.clear_outputs = title_flame_fx_clear_outputs;
    params.try_env = title_flame_fx_try_env;
    params.try_scan = title_flame_fx_try_scan;
    params.trace_fallback_line = "fallback: compiled *_Portable title flame effect assets\n";
    params.use_compat_scan_gate = FALSE;
    params.force_scan_env_var = NULL;
    firered_portable_rom_asset_bind_run(&params);
}

static void title_slash_clear_outputs(void *v)
{
    TitleSlashBindCtx *c = (TitleSlashBindCtx *)v;
    if (c == NULL)
        return;
    *c->out_slash_gfx = NULL;
}

static bool8 title_slash_try_env(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleSlashBindCtx *c = (TitleSlashBindCtx *)user;
    size_t off = 0;
    size_t clen;
    if (!firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_TITLE_SLASH_GFX_OFF", &off))
        return FALSE;
    if (off + TITLE_EFFECT_SIG_LEN > rom_size)
        return FALSE;
    clen = firered_portable_rom_lz77_compressed_size(rom + off, rom_size - off);
    if (clen == 0)
        return FALSE;
    *c->out_slash_gfx = (const u32 *)(rom + off);
    firered_portable_rom_asset_trace_printf(tr, "env bind OK: slash_gfx=0x%zx\n", off);
    firered_portable_rom_asset_trace_flush();
    return TRUE;
}

static bool8 title_slash_try_scan(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    TitleSlashBindCtx *c = (TitleSlashBindCtx *)user;
    const u8 *slash = find_lz_by_signature(rom, rom_size, sSlashGfxLzSig, TITLE_EFFECT_SIG_LEN);
    if (slash == NULL)
        return FALSE;
    *c->out_slash_gfx = (const u32 *)slash;
    firered_portable_rom_asset_trace_printf(tr, "scan bind OK: slash_gfx=0x%zx\n", (size_t)(slash - rom));
    firered_portable_rom_asset_trace_flush();
    return TRUE;
}

void firered_portable_title_screen_try_bind_slash_gfx(const u32 **out_slash_gfx)
{
    TitleSlashBindCtx ctx;
    FireredRomAssetBindParams params;
    *out_slash_gfx = NULL;
    ctx.out_slash_gfx = out_slash_gfx;
    memset(&params, 0, sizeof(params));
    params.trace_env_var = "FIRERED_TRACE_TITLE_ROM";
    params.trace_prefix = "[firered title-rom slash]";
    params.min_rom_size = 0x200u;
    params.user = &ctx;
    params.clear_outputs = title_slash_clear_outputs;
    params.try_env = title_slash_try_env;
    params.try_scan = title_slash_try_scan;
    params.trace_fallback_line = "fallback: compiled *_Portable slash gfx\n";
    params.use_compat_scan_gate = FALSE;
    params.force_scan_env_var = NULL;
    firered_portable_rom_asset_bind_run(&params);
}

void firered_portable_title_core_refresh_provenance_from_rom(void)
{
    const u8 *pal = NULL;
    const u8 *logo_tiles = NULL;
    const u8 *logo_map = NULL;
    const u8 *copyright_tiles = NULL;
    const u8 *copyright_map = NULL;

    firered_portable_title_screen_try_bind_game_title_logo(&pal, &logo_tiles, &logo_map);
    firered_portable_title_screen_try_bind_copyright_press_start(&copyright_tiles, &copyright_map);
}

FireredTitleCoreProvenanceState firered_portable_title_core_get_component_provenance_state(FireredTitleCoreComponent component)
{
    if (component == FIRERED_TITLE_CORE_COMPONENT_GAME_TITLE_LOGO)
        return s_title_core_provenance_game_title_logo;
    if (component == FIRERED_TITLE_CORE_COMPONENT_COPYRIGHT_PRESS_START)
        return s_title_core_provenance_copyright_press_start;
    return FIRERED_TITLE_CORE_PROVENANCE_UNCHANGED;
}

const char *firered_portable_title_core_get_component_name(FireredTitleCoreComponent component)
{
    if (component == FIRERED_TITLE_CORE_COMPONENT_GAME_TITLE_LOGO)
        return "game_title_logo";
    if (component == FIRERED_TITLE_CORE_COMPONENT_COPYRIGHT_PRESS_START)
        return "copyright_press_start";
    return "unknown";
}

const char *firered_portable_title_core_get_provenance_state_name(FireredTitleCoreProvenanceState state)
{
    switch (state)
    {
    case FIRERED_TITLE_CORE_PROVENANCE_UNCHANGED:
        return "UNCHANGED";
    case FIRERED_TITLE_CORE_PROVENANCE_CHANGED_AND_BOUND:
        return "CHANGED_AND_BOUND";
    case FIRERED_TITLE_CORE_PROVENANCE_CHANGED_BUT_FELL_BACK:
        return "CHANGED_BUT_FELL_BACK";
    default:
        return "UNKNOWN";
    }
}
