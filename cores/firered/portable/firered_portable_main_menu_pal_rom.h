#ifndef GUARD_FIRERED_PORTABLE_MAIN_MENU_PAL_ROM_H
#define GUARD_FIRERED_PORTABLE_MAIN_MENU_PAL_ROM_H

#include "global.h"

/*
 * ROM pointers for **main menu** BG palettes (continue / mystery gift screen):
 *   graphics/main_menu/bg.gbapal       — 16 colors (32 bytes)
 *   graphics/main_menu/textbox.gbapal  — 16 colors (32 bytes)
 *
 * Used from `MainMenuGpuInit` (PORTABLE) when bind succeeds; otherwise the
 * compiled `*_Portable` mirrors in `main_menu_portable_assets.h`.
 *
 * Order:
 *   1) Env (both required): FIRERED_ROM_MAIN_MENU_BG_PAL_OFF,
 *      FIRERED_ROM_MAIN_MENU_TEXTBOX_PAL_OFF (hex file offsets into mapped ROM)
 *   2) Built-in profile when both env vars unset (firered_portable_rom_asset_profile_lookup)
 *   3) Signature scan (first 32 bytes of each stock palette) when
 *      firered_portable_rom_compat_should_attempt_rom_asset_scan() is TRUE,
 *      or when FIRERED_ROM_MAIN_MENU_PAL_SCAN is set (non-empty, not "0")
 *
 * Trace: FIRERED_TRACE_MAIN_MENU_PAL_ROM=1 (stderr, narrow).
 */
void firered_portable_main_menu_try_bind_palettes(const u16 **out_bg_pal, const u16 **out_textbox_pal);

#endif /* GUARD_FIRERED_PORTABLE_MAIN_MENU_PAL_ROM_H */
