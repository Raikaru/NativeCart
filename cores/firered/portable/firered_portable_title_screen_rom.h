#ifndef GUARD_FIRERED_PORTABLE_TITLE_SCREEN_ROM_H
#define GUARD_FIRERED_PORTABLE_TITLE_SCREEN_ROM_H

#include "global.h"

/*
 * Resolve pointers into the mapped ROM image for the title-screen "game title
 * logo" trio (8bpp logo BG): palette blob, LZ77 tiles, LZ77 tilemap.
 *
 * On success, sets *out_pal, *out_tiles, *out_map to ENGINE_ROM_ADDR-relative
 * addresses (suitable for LoadPalette / MallocAndDecompress). On failure,
 * leaves all three NULL so callers fall back to *_Portable mirrors.
 *
 * Priority:
 *   1) FIRERED_ROM_TITLE_LOGO_PAL_OFF, FIRERED_ROM_TITLE_LOGO_TILES_OFF,
 *      FIRERED_ROM_TITLE_LOGO_MAP_OFF (hex, strtoul syntax) — for hacks with
 *      moved rodata but known layout.
 *   2) Signature + layout match (vanilla pokefirered-order: 512-byte pal
 *      immediately before tiles LZ; map LZ follows tiles LZ, optional pad).
 *
 * Flow: firered_portable_rom_asset_bind_run (see firered_portable_rom_asset_bind.h).
 */
void firered_portable_title_screen_try_bind_game_title_logo(
    const u8 **out_pal, const u8 **out_tiles, const u8 **out_map);

/*
 * Copyright + "PRESS START" strip on title BG2 (4bpp tiles + tilemap LZ only;
 * palette still comes from gGraphics_TitleScreen_BackgroundPals as in retail).
 *
 * Attempts env then signature scan for every ROM load (same style as game title
 * logo), including vanilla 16 MiB loads.
 *
 * Optional env:
 *   FIRERED_ROM_COPYRIGHT_TILES_OFF, FIRERED_ROM_COPYRIGHT_MAP_OFF (hex offsets)
 * Flow: firered_portable_rom_asset_bind_run (see firered_portable_rom_asset_bind.h).
 */
void firered_portable_title_screen_try_bind_copyright_press_start(const u8 **out_tiles, const u8 **out_map);

/*
 * Box-art mon panel on title BG1 (palette + LZ tiles + LZ map).
 * Optional env: FIRERED_ROM_TITLE_BOX_ART_PAL_OFF, _TILES_OFF, _MAP_OFF.
 */
void firered_portable_title_screen_try_bind_box_art_mon(
    const u8 **out_pal, const u8 **out_tiles, const u8 **out_map);

/*
 * Shared title background palette block (16 colors used by BG14/BG15 paths).
 * Optional env: FIRERED_ROM_TITLE_BG_PALS_OFF.
 */
void firered_portable_title_screen_try_bind_background_pals(const u8 **out_pal);

/*
 * Border background layer on title BG3 (LZ tiles + LZ map).
 * Optional env: FIRERED_ROM_TITLE_BORDER_TILES_OFF, _MAP_OFF.
 */
void firered_portable_title_screen_try_bind_border_bg(const u8 **out_tiles, const u8 **out_map);

/*
 * FireRed title effect sprite assets.
 * Optional env:
 *   FIRERED_ROM_TITLE_FLAMES_PAL_OFF
 *   FIRERED_ROM_TITLE_FLAMES_GFX_OFF
 *   FIRERED_ROM_TITLE_BLANK_FLAMES_GFX_OFF
 */
void firered_portable_title_screen_try_bind_flame_effect_assets(
    const u16 **out_flames_pal, const u32 **out_flames_gfx, const u32 **out_blank_flames_gfx);

/*
 * Optional env: FIRERED_ROM_TITLE_SLASH_GFX_OFF
 */
void firered_portable_title_screen_try_bind_slash_gfx(const u32 **out_slash_gfx);

typedef enum FireredTitleCoreComponent {
    FIRERED_TITLE_CORE_COMPONENT_GAME_TITLE_LOGO = 0,
    FIRERED_TITLE_CORE_COMPONENT_COPYRIGHT_PRESS_START = 1,
} FireredTitleCoreComponent;

typedef enum FireredTitleCoreProvenanceState {
    FIRERED_TITLE_CORE_PROVENANCE_UNCHANGED = 0,
    FIRERED_TITLE_CORE_PROVENANCE_CHANGED_AND_BOUND = 1,
    FIRERED_TITLE_CORE_PROVENANCE_CHANGED_BUT_FELL_BACK = 2,
} FireredTitleCoreProvenanceState;

void firered_portable_title_core_refresh_provenance_from_rom(void);
FireredTitleCoreProvenanceState firered_portable_title_core_get_component_provenance_state(FireredTitleCoreComponent component);
const char *firered_portable_title_core_get_component_name(FireredTitleCoreComponent component);
const char *firered_portable_title_core_get_provenance_state_name(FireredTitleCoreProvenanceState state);

#endif /* GUARD_FIRERED_PORTABLE_TITLE_SCREEN_ROM_H */
