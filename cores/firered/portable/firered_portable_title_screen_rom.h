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
 * ROM scan / bind is skipped for **vanilla 16 MiB retail-like** loads (see
 * firered_portable_rom_compat) so stock behavior stays on compiled *_Portable
 * mirrors. For non-vanilla, expanded, or unknown BPRE-family ROMs, attempts
 * env then signature scan (same style as game title logo).
 *
 * Optional env:
 *   FIRERED_ROM_COPYRIGHT_TILES_OFF, FIRERED_ROM_COPYRIGHT_MAP_OFF (hex offsets)
 *   FIRERED_ROM_COPYRIGHT_PRESS_START_SCAN=1 — force scan even on vanilla (debug)
 *
 * Flow: firered_portable_rom_asset_bind_run (see firered_portable_rom_asset_bind.h).
 */
void firered_portable_title_screen_try_bind_copyright_press_start(const u8 **out_tiles, const u8 **out_map);

#endif /* GUARD_FIRERED_PORTABLE_TITLE_SCREEN_ROM_H */
