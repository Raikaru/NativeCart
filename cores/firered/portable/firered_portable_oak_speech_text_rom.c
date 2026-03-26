#include "portable/firered_portable_oak_speech_text_rom.h"

#include "portable/firered_portable_rom_text_family.h"

#include <stddef.h>

extern const u8 gOakSpeech_Text_AskPlayerGender[];
extern const u8 gOakSpeech_Text_WelcomeToTheWorld[];
extern const u8 gOakSpeech_Text_ThisWorld[];
extern const u8 gOakSpeech_Text_IsInhabitedFarAndWide[];
extern const u8 gOakSpeech_Text_IStudyPokemon[];
extern const u8 gOakSpeech_Text_TellMeALittleAboutYourself[];
extern const u8 gOakSpeech_Text_YourNameWhatIsIt[];
extern const u8 gOakSpeech_Text_SoYourNameIsPlayer[];
extern const u8 gOakSpeech_Text_WhatWasHisName[];
extern const u8 gOakSpeech_Text_YourRivalsNameWhatWasIt[];
extern const u8 gOakSpeech_Text_ConfirmRivalName[];
extern const u8 gOakSpeech_Text_RememberRivalsName[];
extern const u8 gOakSpeech_Text_LetsGo[];

static const char *const sOakTxEnvKeys[FIRERED_OAK_TX_COUNT] = {
    [FIRERED_OAK_TX_AskPlayerGender] = "FIRERED_ROM_OAK_TX_AskPlayerGender",
    [FIRERED_OAK_TX_WelcomeToTheWorld] = "FIRERED_ROM_OAK_TX_WelcomeToTheWorld",
    [FIRERED_OAK_TX_ThisWorld] = "FIRERED_ROM_OAK_TX_ThisWorld",
    [FIRERED_OAK_TX_IsInhabitedFarAndWide] = "FIRERED_ROM_OAK_TX_IsInhabitedFarAndWide",
    [FIRERED_OAK_TX_IStudyPokemon] = "FIRERED_ROM_OAK_TX_IStudyPokemon",
    [FIRERED_OAK_TX_TellMeALittleAboutYourself] = "FIRERED_ROM_OAK_TX_TellMeALittleAboutYourself",
    [FIRERED_OAK_TX_YourNameWhatIsIt] = "FIRERED_ROM_OAK_TX_YourNameWhatIsIt",
    [FIRERED_OAK_TX_SoYourNameIsPlayer] = "FIRERED_ROM_OAK_TX_SoYourNameIsPlayer",
    [FIRERED_OAK_TX_WhatWasHisName] = "FIRERED_ROM_OAK_TX_WhatWasHisName",
    [FIRERED_OAK_TX_YourRivalsNameWhatWasIt] = "FIRERED_ROM_OAK_TX_YourRivalsNameWhatWasIt",
    [FIRERED_OAK_TX_ConfirmRivalName] = "FIRERED_ROM_OAK_TX_ConfirmRivalName",
    [FIRERED_OAK_TX_RememberRivalsName] = "FIRERED_ROM_OAK_TX_RememberRivalsName",
    [FIRERED_OAK_TX_LetsGo] = "FIRERED_ROM_OAK_TX_LetsGo",
};

static const u8 *oak_tx_get_fallback(unsigned entry_index, void *user)
{
    (void)user;

    switch ((FireredOakTxId)entry_index)
    {
    case FIRERED_OAK_TX_AskPlayerGender:
        return gOakSpeech_Text_AskPlayerGender;
    case FIRERED_OAK_TX_WelcomeToTheWorld:
        return gOakSpeech_Text_WelcomeToTheWorld;
    case FIRERED_OAK_TX_ThisWorld:
        return gOakSpeech_Text_ThisWorld;
    case FIRERED_OAK_TX_IsInhabitedFarAndWide:
        return gOakSpeech_Text_IsInhabitedFarAndWide;
    case FIRERED_OAK_TX_IStudyPokemon:
        return gOakSpeech_Text_IStudyPokemon;
    case FIRERED_OAK_TX_TellMeALittleAboutYourself:
        return gOakSpeech_Text_TellMeALittleAboutYourself;
    case FIRERED_OAK_TX_YourNameWhatIsIt:
        return gOakSpeech_Text_YourNameWhatIsIt;
    case FIRERED_OAK_TX_SoYourNameIsPlayer:
        return gOakSpeech_Text_SoYourNameIsPlayer;
    case FIRERED_OAK_TX_WhatWasHisName:
        return gOakSpeech_Text_WhatWasHisName;
    case FIRERED_OAK_TX_YourRivalsNameWhatWasIt:
        return gOakSpeech_Text_YourRivalsNameWhatWasIt;
    case FIRERED_OAK_TX_ConfirmRivalName:
        return gOakSpeech_Text_ConfirmRivalName;
    case FIRERED_OAK_TX_RememberRivalsName:
        return gOakSpeech_Text_RememberRivalsName;
    case FIRERED_OAK_TX_LetsGo:
        return gOakSpeech_Text_LetsGo;
    default:
        return gOakSpeech_Text_WelcomeToTheWorld;
    }
}

static const FireredRomTextFamilyParams sOakTxParams = {
    .trace_env_var = "FIRERED_TRACE_OAK_SPEECH_ROM",
    .trace_prefix = "[firered oak-tx-rom]",
    .trace_bind_detail_env_var = "FIRERED_TRACE_EARLY_INTRO_FLOW",
    .entry_count = FIRERED_OAK_TX_COUNT,
    .env_key_names = sOakTxEnvKeys,
    .min_rom_size_for_scan = 0x200u,
    .env_offset_max_eos_search = 2048u,
    .scan_min_match_len = 12u,
    .get_fallback = oak_tx_get_fallback,
    .try_profile_rom_off = NULL,
    .try_scan = NULL,
    .user = NULL,
    .trace_warn_multi_scan = TRUE,
};

static const u8 *sOakTxCache[FIRERED_OAK_TX_COUNT];
static u8 sOakTxBound;

const u8 *firered_portable_oak_speech_get_text(FireredOakTxId id)
{
    if ((unsigned)id >= (unsigned)FIRERED_OAK_TX_COUNT)
        return oak_tx_get_fallback(FIRERED_OAK_TX_WelcomeToTheWorld, NULL);

    if (!sOakTxBound)
    {
        sOakTxBound = 1;
        firered_portable_rom_text_family_bind_all(&sOakTxParams, sOakTxCache);
    }

    return sOakTxCache[id];
}
