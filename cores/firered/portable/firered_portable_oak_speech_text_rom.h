#ifndef GUARD_FIRERED_PORTABLE_OAK_SPEECH_TEXT_ROM_H
#define GUARD_FIRERED_PORTABLE_OAK_SPEECH_TEXT_ROM_H

#include "global.h"

/*
 * ROM-backed Oak intro dialogue (gOakSpeech_Text_* family) for PORTABLE/SDL.
 *
 * Uses the shared `firered_portable_rom_text_family` flow (see
 * firered_portable_rom_text_family.h):
 *   1) Optional env override FIRERED_ROM_OAK_TX_<Suffix> (hex file offset, strtoul).
 *   2) Optional profile offset (none wired for Oak today; set try_profile in family params).
 *   3) Else scan the ROM for a byte-for-byte match vs the compiled decomp string
 *      (same encoding as retail; works when hack text unchanged but relocated).
 *   4) Else use the compiled string (fallback).
 *
 * Tracing (stderr, not NDEBUG-gated): set FIRERED_TRACE_OAK_SPEECH_ROM (non-empty, not "0")
 * for a one-line summary after the first resolution pass.
 */
typedef enum {
    FIRERED_OAK_TX_AskPlayerGender = 0,
    FIRERED_OAK_TX_WelcomeToTheWorld,
    FIRERED_OAK_TX_ThisWorld,
    FIRERED_OAK_TX_IsInhabitedFarAndWide,
    FIRERED_OAK_TX_IStudyPokemon,
    FIRERED_OAK_TX_TellMeALittleAboutYourself,
    FIRERED_OAK_TX_YourNameWhatIsIt,
    FIRERED_OAK_TX_SoYourNameIsPlayer,
    FIRERED_OAK_TX_WhatWasHisName,
    FIRERED_OAK_TX_YourRivalsNameWhatWasIt,
    FIRERED_OAK_TX_ConfirmRivalName,
    FIRERED_OAK_TX_RememberRivalsName,
    FIRERED_OAK_TX_LetsGo,
    FIRERED_OAK_TX_COUNT
} FireredOakTxId;

const u8 *firered_portable_oak_speech_get_text(FireredOakTxId id);

#endif /* GUARD_FIRERED_PORTABLE_OAK_SPEECH_TEXT_ROM_H */
