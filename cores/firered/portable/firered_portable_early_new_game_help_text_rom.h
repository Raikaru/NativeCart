#ifndef GUARD_FIRERED_PORTABLE_EARLY_NEW_GAME_HELP_TEXT_ROM_H
#define GUARD_FIRERED_PORTABLE_EARLY_NEW_GAME_HELP_TEXT_ROM_H

#include "global.h"

/*
 * ROM-backed strings for the **controls guide** (pages 1–3) and **Pikachu intro** text pages
 * shown in `StartNewGameScene` before Oak’s spoken dialogue.
 *
 * Same resolution order as Oak intro text (`firered_portable_rom_text_family`):
 *   env hex offset → scan vs compiled fallback bytes → compiled fallback.
 *
 * Trace: `FIRERED_TRACE_EARLY_NEW_GAME_HELP_ROM` (non-empty, not "0").
 *
 * Scene + bind detail: `FIRERED_TRACE_EARLY_INTRO_FLOW` (non-empty, not "0") enables
 * per-entry `bind_detail` lines when this family binds (see `firered_portable_rom_text_family`)
 * and `firered_portable_early_intro_flow_trace_snapshot()` call sites in `oak_speech.c`.
 */
typedef enum {
    FIRERED_EARLY_NG_TX_Intro = 0,
    FIRERED_EARLY_NG_TX_DPad,
    FIRERED_EARLY_NG_TX_AButton,
    FIRERED_EARLY_NG_TX_BButton,
    FIRERED_EARLY_NG_TX_StartButton,
    FIRERED_EARLY_NG_TX_SelectButton,
    FIRERED_EARLY_NG_TX_LRButtons,
    FIRERED_EARLY_NG_TX_PikachuPage1,
    FIRERED_EARLY_NG_TX_PikachuPage2,
    FIRERED_EARLY_NG_TX_PikachuPage3,
    FIRERED_EARLY_NG_TX_COUNT
} FireredEarlyNewGameHelpTxId;

const u8 *firered_portable_early_new_game_help_get_text(FireredEarlyNewGameHelpTxId id);

void firered_portable_early_intro_flow_trace_snapshot(const char *where, FireredEarlyNewGameHelpTxId id, const u8 *text);

#endif /* GUARD_FIRERED_PORTABLE_EARLY_NEW_GAME_HELP_TEXT_ROM_H */
