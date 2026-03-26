#ifndef GUARD_FIRERED_PORTABLE_NEW_GAME_INTRO_PROSE_ROM_H
#define GUARD_FIRERED_PORTABLE_NEW_GAME_INTRO_PROSE_ROM_H

#include "global.h"

/*
 * Optional **generic** ROM-backed new-game **intro prose** pages (PORTABLE only).
 *
 * When both env vars below are set and validate, `StartNewGameScene` may show N
 * full-screen text pages (same UI pattern as Pikachu intro) **before** the vanilla
 * controls guide + Pikachu flow. When inactive or invalid, behavior is unchanged.
 *
 * Contract:
 *   - `FIRERED_ROM_NEW_GAME_INTRO_PROSE_PTR_TABLE_OFF` — hex **file offset** of a
 *     contiguous array of `u32` little-endian ROM addresses (`0x08XXXXXX` each).
 *   - `FIRERED_ROM_NEW_GAME_INTRO_PROSE_PAGE_COUNT` — hex or decimal page count
 *     (via `strtoul(..., 0)`), 1 .. FIRERED_NEW_GAME_INTRO_PROSE_MAX_PAGES.
 *   - Each pointer’s **file offset** (`ptr - 0x08000000`) must lie in the loaded ROM;
 *     each string must reach **EOS** (0xFF) within FIRERED_NEW_GAME_INTRO_PROSE_MAX_STR bytes.
 *
 * **Binding order (PORTABLE):**
 *   1. Env **`FIRERED_ROM_NEW_GAME_INTRO_PROSE_PTR_TABLE_OFF`** + **`…_PAGE_COUNT`** (both required).
 *   2. Else a **profile** row in `firered_portable_new_game_intro_prose_rom.c` keyed by
 *      `firered_portable_rom_compat_get()` (**IEEE CRC32** of the full ROM image and/or
 *      `profile_id`) — same idea as `FireredRomU32TableProfileRow` for layout packs.
 *
 * No hack-specific parsing: this is a plain pointer table only. Unknown ROMs still use env
 * or tooling (e.g. `tools/append_intro_prose_ptr_table_to_rom.py`).
 */

#define FIRERED_NEW_GAME_INTRO_PROSE_MAX_PAGES 16u
#define FIRERED_NEW_GAME_INTRO_PROSE_MAX_STR 8192u

void firered_portable_new_game_intro_prose_rom_refresh_after_rom_load(void);
bool8 firered_portable_new_game_intro_prose_rom_should_run(void);
unsigned firered_portable_new_game_intro_prose_rom_page_count(void);
const u8 *firered_portable_new_game_intro_prose_rom_get_page(unsigned page_index);

#endif /* GUARD_FIRERED_PORTABLE_NEW_GAME_INTRO_PROSE_ROM_H */
