#include "global.h"

#include "portable/firered_portable_main_menu_pal_rom.h"

#include "portable/firered_portable_rom_asset_bind.h"
#include "portable/firered_portable_rom_asset_profile.h"
#include "engine_internal.h"

#include <stddef.h>
#include <string.h>

enum {
    MAIN_MENU_PAL_BYTES = 32u,
};

/* graphics/main_menu/bg.gbapal (FireRed stock, 16 colors LE) */
static const u8 sMainMenuBgPalSig[MAIN_MENU_PAL_BYTES] = {
    0x51, 0x7e, 0xff, 0x7f, 0xe6, 0x28, 0x8b, 0x39, 0x21, 0x08, 0x72, 0x56, 0x79, 0x77, 0xd6, 0x5e,
    0x37, 0x6f, 0x84, 0x18, 0x0f, 0x46, 0x46, 0x3d, 0xe5, 0x61, 0x27, 0x6a, 0x8b, 0x72, 0x11, 0x7b,
};

/* graphics/main_menu/textbox.gbapal (FireRed stock) */
static const u8 sMainMenuTextboxPalSig[MAIN_MENU_PAL_BYTES] = {
    0xff, 0x7f, 0xff, 0x7f, 0x8c, 0x31, 0x5a, 0x67, 0x3c, 0x04, 0xff, 0x3a, 0x64, 0x06, 0xd2, 0x4b,
    0x46, 0x65, 0x14, 0x7b, 0xff, 0x7f, 0x8c, 0x31, 0x5a, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

typedef struct MainMenuPalBindCtx {
    const u16 **out_bg;
    const u16 **out_textbox;
} MainMenuPalBindCtx;

static void main_menu_pal_clear_outputs(void *v)
{
    MainMenuPalBindCtx *c = (MainMenuPalBindCtx *)v;

    if (c == NULL)
        return;
    *c->out_bg = NULL;
    *c->out_textbox = NULL;
}

static void main_menu_pal_trace_scan_begin(void *user, size_t rom_size, FireredRomAssetTrace *tr)
{
    (void)user;
    firered_portable_rom_asset_trace_printf(tr, "bind attempt: rom_size=0x%zx scan=1\n", rom_size);
    firered_portable_rom_asset_trace_flush();
}

static bool8 main_menu_pal_try_env(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    MainMenuPalBindCtx *c = (MainMenuPalBindCtx *)user;
    size_t bg_off = 0, tb_off = 0;
    int ok_bg, ok_tb;
    FireredRomAssetProfileOffsets prof;
    uint32_t need;

    ok_bg = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_MAIN_MENU_BG_PAL_OFF", &bg_off);
    ok_tb = firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_MAIN_MENU_TEXTBOX_PAL_OFF", &tb_off);

    if (ok_bg && ok_tb)
    {
        if (bg_off + MAIN_MENU_PAL_BYTES > rom_size || tb_off + MAIN_MENU_PAL_BYTES > rom_size)
        {
            firered_portable_rom_asset_trace_printf(tr, "env bind out of range (rom_size=0x%zx)\n", rom_size);
            firered_portable_rom_asset_trace_flush();
            return FALSE;
        }

        *c->out_bg = (const u16 *)(rom + bg_off);
        *c->out_textbox = (const u16 *)(rom + tb_off);

        firered_portable_rom_asset_trace_printf(tr, "env bind OK: bg=0x%zx textbox=0x%zx\n", bg_off, tb_off);
        firered_portable_rom_asset_trace_flush();
        return TRUE;
    }

    if (!ok_bg && !ok_tb && firered_portable_rom_asset_profile_lookup(FIRERED_ROM_ASSET_FAMILY_MAIN_MENU_PALS, &prof))
    {
        need = FIRERED_ROM_ASSET_PROFILE_HAS_PAL | FIRERED_ROM_ASSET_PROFILE_HAS_PAL2;
        if ((prof.set_mask & need) == need)
        {
            bg_off = prof.pal_off;
            tb_off = prof.pal2_off;
            if (bg_off + MAIN_MENU_PAL_BYTES > rom_size || tb_off + MAIN_MENU_PAL_BYTES > rom_size)
            {
                firered_portable_rom_asset_trace_printf(tr, "profile bind out of range (rom_size=0x%zx)\n", rom_size);
                firered_portable_rom_asset_trace_flush();
                return FALSE;
            }

            *c->out_bg = (const u16 *)(rom + bg_off);
            *c->out_textbox = (const u16 *)(rom + tb_off);

            firered_portable_rom_asset_trace_printf(tr, "profile bind OK: bg=0x%zx textbox=0x%zx\n", bg_off, tb_off);
            firered_portable_rom_asset_trace_flush();
            return TRUE;
        }
    }

    firered_portable_rom_asset_trace_printf(tr,
        "env/profile bind incomplete: need both env vars or full profile (BG_PAL + TEXTBOX_PAL)\n");
    firered_portable_rom_asset_trace_flush();
    return FALSE;
}

static bool8 main_menu_pal_try_scan(void *user, const u8 *rom, size_t rom_size, FireredRomAssetTrace *tr)
{
    MainMenuPalBindCtx *c = (MainMenuPalBindCtx *)user;
    size_t i;
    int have_bg = 0, have_tb = 0;
    size_t bg_off = 0, tb_off = 0;

    if (rom_size < MAIN_MENU_PAL_BYTES)
        return FALSE;

    for (i = 0; i + MAIN_MENU_PAL_BYTES <= rom_size; i += 2)
    {
        if (!have_bg && memcmp(rom + i, sMainMenuBgPalSig, MAIN_MENU_PAL_BYTES) == 0)
        {
            have_bg = 1;
            bg_off = i;
        }
        if (!have_tb && memcmp(rom + i, sMainMenuTextboxPalSig, MAIN_MENU_PAL_BYTES) == 0)
        {
            have_tb = 1;
            tb_off = i;
        }
        if (have_bg && have_tb)
            break;
    }

    if (!have_bg || !have_tb)
    {
        firered_portable_rom_asset_trace_printf(tr, "scan: missing match (bg=%d textbox=%d)\n", have_bg, have_tb);
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    if (bg_off == tb_off)
    {
        firered_portable_rom_asset_trace_printf(tr, "scan: bg/textbox offsets collide at 0x%zx\n", bg_off);
        firered_portable_rom_asset_trace_flush();
        return FALSE;
    }

    *c->out_bg = (const u16 *)(rom + bg_off);
    *c->out_textbox = (const u16 *)(rom + tb_off);

    firered_portable_rom_asset_trace_printf(tr, "scan bind OK: bg=0x%zx textbox=0x%zx\n", bg_off, tb_off);
    firered_portable_rom_asset_trace_flush();
    return TRUE;
}

void firered_portable_main_menu_try_bind_palettes(const u16 **out_bg_pal, const u16 **out_textbox_pal)
{
    MainMenuPalBindCtx ctx;
    FireredRomAssetBindParams params;

    if (out_bg_pal == NULL || out_textbox_pal == NULL)
        return;

    *out_bg_pal = NULL;
    *out_textbox_pal = NULL;

    ctx.out_bg = out_bg_pal;
    ctx.out_textbox = out_textbox_pal;

    memset(&params, 0, sizeof(params));
    params.trace_env_var = "FIRERED_TRACE_MAIN_MENU_PAL_ROM";
    params.trace_prefix = "[firered main-menu-pal-rom]";
    params.min_rom_size = 0x200u;
    params.user = &ctx;
    params.clear_outputs = main_menu_pal_clear_outputs;
    params.try_env = main_menu_pal_try_env;
    params.try_scan = main_menu_pal_try_scan;
    params.trace_scan_beginning = main_menu_pal_trace_scan_begin;
    params.trace_skip_scan_line =
        "skip ROM scan (vanilla 16MiB retail-like compat profile); using *_Portable main menu palettes\n";
    params.trace_fallback_line = "fallback: compiled *_Portable main menu palettes\n";
    params.use_compat_scan_gate = TRUE;
    params.force_scan_env_var = "FIRERED_ROM_MAIN_MENU_PAL_SCAN";

    firered_portable_rom_asset_bind_run(&params);
}
