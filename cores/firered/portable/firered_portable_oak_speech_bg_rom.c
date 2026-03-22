#include "global.h"

#include "portable/firered_portable_oak_speech_bg_rom.h"

#include "portable/firered_portable_rom_asset_bind.h"
#include "portable/firered_portable_rom_asset_profile.h"
#include "portable/firered_portable_rom_lz_asset.h"
#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
    OAK_BG_TILES_SIG_LEN = 32,
    OAK_BG_MAP_SIG_LEN = 32,
    OAK_BG_MAP_DECOMP_SIZE = 0x500u,
    OAK_BG_MAP_SEARCH_MAX_AFTER_TILES = 256u * 1024u,
    OAK_BG_PAL_SIZE = 512u,
    MAX_OAK_BG_TILE_MATCHES = 16u,
};

typedef struct OakBgBindCtx {
    const u8 **out_pal;
    const u8 **out_tiles;
    const u8 **out_map;
} OakBgBindCtx;

/* graphics/oak_speech/oak_speech_bg.4bpp.lz (FireRed stock) */
static const u8 sOakBgTilesLzSig[OAK_BG_TILES_SIG_LEN] = {
    0x10, 0x40, 0x01, 0x00, 0x3c, 0x00, 0x00, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0x50, 0x01, 0x11,
    0x11, 0xcc, 0xf0, 0x01, 0x90, 0x01, 0x22, 0x22, 0xb0, 0x01, 0xd0, 0x1f, 0x44, 0x44, 0x92, 0xb0,
};

/* graphics/oak_speech/oak_speech_bg.bin.lz */
static const u8 sOakBgMapLzSig[OAK_BG_MAP_SIG_LEN] = {
    0x10, 0x00, 0x05, 0x00, 0x3f, 0x08, 0x00, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0xf0,
    0x01, 0xf0, 0x01, 0xff, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01, 0xf0, 0x01,
};

static void oak_bg_clear_outputs(void *v)
{
    OakBgBindCtx *c = (OakBgBindCtx *)v;

    if (c == NULL)
        return;
    *c->out_pal = NULL;
    *c->out_tiles = NULL;
    *c->out_map = NULL;
}

static void oak_bg_trace_scan_begin(void *user, size_t rom_size, FireredRomAssetTrace *tr)
{
    (void)user;
    firered_portable_rom_asset_trace_printf(tr, "bind attempt: rom_size=0x%zx scan=1\n", rom_size);
    firered_portable_rom_asset_trace_flush();
}

static bool8 oak_bg_try_env(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    OakBgBindCtx *c = (OakBgBindCtx *)user;
    size_t tiles_off = 0, map_off = 0, pal_off = 0;
    int ok_tiles, ok_map, ok_pal;
    FireredRomAssetProfileOffsets prof;
    uint32_t need;
    int ok_profile_pal;

    ok_tiles = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_OAK_SPEECH_BG_TILES_OFF", &tiles_off);
    ok_map = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_OAK_SPEECH_BG_MAP_OFF", &map_off);
    ok_pal = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_OAK_SPEECH_BG_PAL_OFF", &pal_off);

    if (ok_tiles && ok_map)
    {
        if (tiles_off + OAK_BG_TILES_SIG_LEN > rom_size || map_off + OAK_BG_MAP_SIG_LEN > rom_size)
        {
            firered_portable_rom_asset_trace_printf(tr, "env bind out of range (rom_size=0x%zx)\n", rom_size);
            firered_portable_rom_asset_trace_flush();
            return FALSE;
        }

        if (ok_pal && pal_off + OAK_BG_PAL_SIZE > rom_size)
        {
            firered_portable_rom_asset_trace_printf(tr, "env PAL_OFF out of range\n");
            firered_portable_rom_asset_trace_flush();
            ok_pal = 0;
        }

        *c->out_tiles = rom + tiles_off;
        *c->out_map = rom + map_off;

        ok_profile_pal = 0;
        if (ok_pal)
            *c->out_pal = rom + pal_off;
        else if (firered_portable_rom_asset_profile_lookup(FIRERED_ROM_ASSET_FAMILY_OAK_MAIN_BG, &prof)
            && (prof.set_mask & FIRERED_ROM_ASSET_PROFILE_HAS_PAL) != 0u
            && prof.pal_off + OAK_BG_PAL_SIZE <= rom_size)
        {
            *c->out_pal = rom + prof.pal_off;
            ok_profile_pal = 1;
        }
        else
            *c->out_pal = NULL;

        firered_portable_rom_asset_trace_printf(tr, "env bind OK: tiles=0x%zx map=0x%zx pal=%s\n", tiles_off, map_off,
            ok_pal ? "env" : (ok_profile_pal ? "profile" : "unset(use_Portable_or_scan)"));
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }

    if (!ok_tiles && !ok_map
        && firered_portable_rom_asset_profile_lookup(FIRERED_ROM_ASSET_FAMILY_OAK_MAIN_BG, &prof))
    {
        need = FIRERED_ROM_ASSET_PROFILE_HAS_TILES | FIRERED_ROM_ASSET_PROFILE_HAS_MAP;
        if ((prof.set_mask & need) == need)
        {
            tiles_off = prof.tiles_off;
            map_off = prof.map_off;
            if (tiles_off + OAK_BG_TILES_SIG_LEN > rom_size || map_off + OAK_BG_MAP_SIG_LEN > rom_size)
            {
                firered_portable_rom_asset_trace_printf(tr, "profile bind out of range (rom_size=0x%zx)\n", rom_size);
                firered_portable_rom_asset_trace_flush();
                return FALSE;
            }

            *c->out_tiles = rom + tiles_off;
            *c->out_map = rom + map_off;

            if (ok_pal && pal_off + OAK_BG_PAL_SIZE <= rom_size)
                *c->out_pal = rom + pal_off;
            else if ((prof.set_mask & FIRERED_ROM_ASSET_PROFILE_HAS_PAL) != 0u
                && prof.pal_off + OAK_BG_PAL_SIZE <= rom_size)
                *c->out_pal = rom + prof.pal_off;
            else
                *c->out_pal = NULL;

            firered_portable_rom_asset_trace_printf(tr, "profile bind OK: tiles=0x%zx map=0x%zx pal=%s\n", tiles_off,
                map_off, *c->out_pal ? "set" : "unset");
            firered_portable_rom_asset_trace_flush();
            return TRUE;
        }
    }

    firered_portable_rom_asset_trace_printf(tr, "env/profile bind incomplete: need TILES_OFF and MAP_OFF (or profile)\n");
    firered_portable_rom_asset_trace_flush();
    return FALSE;
}

static bool8 oak_bg_try_scan(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    OakBgBindCtx *c = (OakBgBindCtx *)user;
    size_t tile_match_offs[MAX_OAK_BG_TILE_MATCHES];
    unsigned tile_match_count = 0u;
    size_t i;
    unsigned fail_lz = 0u, fail_map = 0u;
    int saw_uncapped = 0;

    if (rom_size < OAK_BG_TILES_SIG_LEN + OAK_BG_MAP_SIG_LEN)
        return FALSE;

    for (i = 0; i + OAK_BG_TILES_SIG_LEN <= rom_size; i += 4)
    {
        if (memcmp(rom + i, sOakBgTilesLzSig, OAK_BG_TILES_SIG_LEN) != 0)
            continue;

        if (tile_match_count < MAX_OAK_BG_TILE_MATCHES)
            tile_match_offs[tile_match_count++] = i;
        else
            saw_uncapped = 1;
    }

    firered_portable_rom_asset_trace_printf(tr, "scan: tile LZ hits=%u%s\n", tile_match_count,
        saw_uncapped ? " (capped)" : "");
    firered_portable_rom_asset_trace_flush();

    if (tile_match_count == 0u)
    {
        firered_portable_rom_asset_trace_printf(tr, "scan: no oak_speech_bg tiles signature match\n");
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    for (i = 0; i < tile_match_count; i++)
    {
        const u8 *tiles = rom + tile_match_offs[i];
        size_t tiles_avail = rom_size - tile_match_offs[i];
        size_t tiles_clen = firered_portable_rom_lz77_compressed_size(tiles, tiles_avail);
        FireredRomLzFollowupKind map_kind = FIRERED_ROM_LZ_FOLLOWUP_NONE;
        const u8 *map;

        if (tiles_clen == 0)
        {
            fail_lz++;
            continue;
        }

        map = firered_portable_rom_find_lz_followup(tiles + tiles_clen, (size_t)((rom + rom_size) - (tiles + tiles_clen)),
            OAK_BG_MAP_SEARCH_MAX_AFTER_TILES, sOakBgMapLzSig, OAK_BG_MAP_SIG_LEN, OAK_BG_MAP_DECOMP_SIZE, &map_kind);

        if (map == NULL)
        {
            fail_map++;
            continue;
        }

        *c->out_tiles = tiles;
        *c->out_map = map;
        *c->out_pal = NULL;

        firered_portable_rom_asset_trace_printf(tr,
            "scan bind OK: tiles=0x%zx map=0x%zx tiles_clen=0x%zx map_match=%s\n", (size_t)(tiles - rom),
            (size_t)(map - rom), tiles_clen,
            map_kind == FIRERED_ROM_LZ_FOLLOWUP_STRICT_SIG ? "strict_32b" : "lz_dec0x500");
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }

    firered_portable_rom_asset_trace_printf(tr, "scan: no valid pair (lz_fail=%u map_fail=%u)\n", fail_lz, fail_map);
    firered_portable_rom_asset_trace_flush();
    return FALSE;
}

void firered_portable_oak_speech_try_bind_main_bg(const u8 **out_pal, const u8 **out_tiles, const u8 **out_map)
{
    OakBgBindCtx ctx;
    FireredRomAssetBindParams params;

    *out_pal = NULL;
    *out_tiles = NULL;
    *out_map = NULL;

    ctx.out_pal = out_pal;
    ctx.out_tiles = out_tiles;
    ctx.out_map = out_map;

    memset(&params, 0, sizeof(params));
    params.trace_env_var = "FIRERED_TRACE_OAK_BG_ROM";
    params.trace_prefix = "[firered oak-bg-rom]";
    params.min_rom_size = 0x200u;
    params.user = &ctx;
    params.clear_outputs = oak_bg_clear_outputs;
    params.try_env = oak_bg_try_env;
    params.try_scan = oak_bg_try_scan;
    params.trace_scan_beginning = oak_bg_trace_scan_begin;
    params.trace_skip_scan_line = "skip ROM scan (vanilla-like compat); using *_Portable oak_speech_bg\n";
    params.trace_fallback_line = "fallback: compiled *_Portable oak_speech_bg\n";
    params.use_compat_scan_gate = TRUE;
    params.force_scan_env_var = NULL;

    firered_portable_rom_asset_bind_run(&params);
}
