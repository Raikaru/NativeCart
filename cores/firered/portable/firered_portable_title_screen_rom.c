#include "portable/firered_portable_title_screen_rom.h"

#include "portable/firered_portable_rom_compat.h"
#include "portable/firered_portable_rom_asset_profile.h"
#include "portable/firered_portable_rom_asset_bind.h"
#include "portable/firered_portable_rom_lz_asset.h"
#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    TITLE_LOGO_TILE_SIG_LEN = 32,
    TITLE_LOGO_MAP_SIG_LEN = 32,
    TITLE_LOGO_PAL_PREFIX_LEN = 32,
    TITLE_LOGO_PAL_SIZE = 512,
    /* Vanilla game_title_logo.bin uncompressed size (tilemap). */
    TITLE_LOGO_MAP_DECOMP_SIZE = 0x500u,
    /*
     * Previously only ~8KiB after compressed tiles was scanned, with a strict
     * 32-byte match on the stock compressed map — too tight for hacks that pad
     * or reorder rodata (e.g. FRLG+). Widen search; validate relaxed hits by
     * LZ header + dec size + parseable stream.
     */
    TITLE_LOGO_MAP_SEARCH_MAX_AFTER_TILES = 256u * 1024u,
    MAX_TILE_MATCHES = 24u,
};

enum {
    TITLE_LOGO_MAP_MATCH_NONE = 0,
    TITLE_LOGO_MAP_MATCH_STRICT_32 = 1,
    TITLE_LOGO_MAP_MATCH_LZ_DEC0x500 = 2,
};

enum {
    COPYRIGHT_TILES_SIG_LEN = 32,
    COPYRIGHT_MAP_SIG_LEN = 32,
    COPYRIGHT_MAP_DECOMP_SIZE = 0x500u,
    COPYRIGHT_MAP_SEARCH_MAX_AFTER_TILES = 256u * 1024u,
    MAX_COPYRIGHT_TILE_MATCHES = 16u,
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

static const u8 sTitleLogoPalPrefix[TITLE_LOGO_PAL_PREFIX_LEN] = {
    0xe0, 0x17, 0x00, 0x00, 0xbf, 0x0b, 0x7e, 0x17, 0xbe, 0x7b, 0xdf, 0x0f, 0xde, 0x17, 0xde,
    0x7b, 0xfe, 0x7f, 0xdf, 0x7f, 0x04, 0x7c, 0x0d, 0x4d, 0x04, 0x74, 0x7e, 0x0f, 0x7e, 0x13,
    0xbf, 0x17
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
    unsigned fail_lz = 0u, fail_pal_room = 0u, fail_pal_sig = 0u, fail_map_layout = 0u;
    int saw_uncapped_tile_hits = 0;
    size_t ex_lz = (size_t)-1, ex_room = (size_t)-1, ex_pal = (size_t)-1, ex_map = (size_t)-1;

    if (rom_size < TITLE_LOGO_PAL_SIZE + TITLE_LOGO_TILE_SIG_LEN + TITLE_LOGO_MAP_SIG_LEN)
    {
        firered_portable_rom_asset_trace_printf(tr,
            "scan skipped: rom_size=0x%zx smaller than minimum for logo scan\n", rom_size);
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

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
        firered_portable_rom_asset_trace_printf(tr,
            "scan: no tiles signature match (repacked/moved title logo or different asset layout)\n");
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    for (i = 0; i < tile_match_count; i++)
    {
        const u8 *tiles = rom + tile_match_offs[i];
        size_t tiles_avail = rom_size - tile_match_offs[i];
        size_t tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, tiles_avail);
        const u8 *pal;
        const u8 *map;
        int map_kind = TITLE_LOGO_MAP_MATCH_NONE;

        if (tiles_clen == 0)
        {
            fail_lz++;
            if (ex_lz == (size_t)-1)
                ex_lz = tile_match_offs[i];
            continue;
        }

        if (tile_match_offs[i] < TITLE_LOGO_PAL_SIZE)
        {
            fail_pal_room++;
            if (ex_room == (size_t)-1)
                ex_room = tile_match_offs[i];
            continue;
        }

        pal = tiles - TITLE_LOGO_PAL_SIZE;
        if (memcmp(pal, sTitleLogoPalPrefix, TITLE_LOGO_PAL_PREFIX_LEN) != 0)
        {
            fail_pal_sig++;
            if (ex_pal == (size_t)-1)
                ex_pal = tile_match_offs[i];
            continue;
        }

        map = find_map_after_tiles(rom, rom_size, tiles, tiles_clen, &map_kind);
        if (map == NULL)
        {
            fail_map_layout++;
            if (ex_map == (size_t)-1)
                ex_map = tile_match_offs[i];
            continue;
        }

        *c->out_pal = pal;
        *c->out_tiles = tiles;
        *c->out_map = map;

        firered_portable_rom_asset_trace_printf(tr,
            "scan bind OK: pal=0x%zx tiles=0x%zx map=0x%zx (tiles_clen=0x%zx map_match=%s)\n",
            (size_t)(pal - rom), (size_t)(tiles - rom), (size_t)(map - rom), tiles_clen,
            map_kind == TITLE_LOGO_MAP_MATCH_STRICT_32 ? "strict_32b" : "lz_dec0x500");
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }

    {
        char aggregate[384];
        size_t pos = 0;
        int nw;

        nw = snprintf(aggregate + pos, sizeof aggregate - pos,
            "scan: no valid triple (lz_fail=%u pal_room_fail=%u pal_sig_fail=%u map_layout_fail=%u)", fail_lz,
            fail_pal_room, fail_pal_sig, fail_map_layout);
        if (nw > 0)
            pos += (size_t)nw;

        if (ex_lz != (size_t)-1 || ex_room != (size_t)-1 || ex_pal != (size_t)-1 || ex_map != (size_t)-1)
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
            if (ex_pal != (size_t)-1)
            {
                nw = snprintf(aggregate + pos, sizeof aggregate - pos, " pal_sig=0x%zx", ex_pal);
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
        firered_portable_rom_asset_trace_printf(tr, "scan: no tiles signature match\n");
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    for (i = 0; i < tile_match_count; i++)
    {
        const u8 *tiles = rom + tile_match_offs[i];
        size_t tiles_avail = rom_size - tile_match_offs[i];
        size_t tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, tiles_avail);
        const u8 *map;
        int map_kind = COPYRIGHT_MAP_MATCH_NONE;

        if (tiles_clen == 0)
        {
            fail_lz++;
            continue;
        }

        map = find_copyright_map_after_tiles(rom, rom_size, tiles, tiles_clen, &map_kind);
        if (map == NULL)
        {
            fail_map++;
            continue;
        }

        *c->out_tiles = tiles;
        *c->out_map = map;

        firered_portable_rom_asset_trace_printf(tr,
            "scan bind OK: tiles=0x%zx map=0x%zx tiles_clen=0x%zx map_match=%s\n", (size_t)(tiles - rom),
            (size_t)(map - rom), tiles_clen,
            map_kind == COPYRIGHT_MAP_MATCH_STRICT_32 ? "strict_32b" : "lz_dec0x500");
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

    firered_portable_rom_asset_bind_run(&params);
}

void firered_portable_title_screen_try_bind_copyright_press_start(const u8 **out_tiles, const u8 **out_map)
{
    TitleCopyrightBindCtx ctx;
    FireredRomAssetBindParams params;

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
    params.trace_skip_scan_line =
        "skip ROM scan (vanilla 16MiB retail-like compat profile); "
        "using *_Portable copyright/press-start graphics\n";
    params.trace_fallback_line = "fallback: compiled *_Portable copyright/press-start graphics\n";
    params.use_compat_scan_gate = TRUE;
    params.force_scan_env_var = "FIRERED_ROM_COPYRIGHT_PRESS_START_SCAN";

    firered_portable_rom_asset_bind_run(&params);
}
