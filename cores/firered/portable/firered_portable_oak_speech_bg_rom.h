#ifndef GUARD_FIRERED_PORTABLE_OAK_SPEECH_BG_ROM_H
#define GUARD_FIRERED_PORTABLE_OAK_SPEECH_BG_ROM_H

#include "global.h"

/*
 * ROM pointers for Oak intro / naming-return **main background** (BG1):
 *   graphics/oak_speech/oak_speech_bg.4bpp.lz
 *   graphics/oak_speech/oak_speech_bg.bin.lz
 * plus optional palette (normally graphics/oak_speech/bg_tiles.gbapal) when
 * FIRERED_ROM_OAK_SPEECH_BG_PAL_OFF is set — stock ROM does not lay palette
 * immediately before these tiles, so scan mode binds tiles+map only unless
 * env supplies PAL_OFF.
 *
 * On success, pointers are valid for MallocAndDecompress / CopyToBgTilemapBuffer /
 * LoadPalette. On failure, all outputs NULL → use *_Portable mirrors.
 *
 * Order:
 *   1) Env: FIRERED_ROM_OAK_SPEECH_BG_TILES_OFF, _MAP_OFF (hex); optional _PAL_OFF
 *   2) Built-in profile row (firered_portable_rom_asset_profile_lookup): full tiles+map when env
 *      omits both; optional palette when env gives tiles+map but omits PAL_OFF
 *   3) LZ signature scan after tiles (shared helpers) if
 *      firered_portable_rom_compat_should_attempt_rom_asset_scan() is TRUE
 *
 * Trace: FIRERED_TRACE_OAK_BG_ROM=1 (stderr, narrow).
 */
void firered_portable_oak_speech_try_bind_main_bg(const u8 **out_pal, const u8 **out_tiles, const u8 **out_map);

#endif /* GUARD_FIRERED_PORTABLE_OAK_SPEECH_BG_ROM_H */
