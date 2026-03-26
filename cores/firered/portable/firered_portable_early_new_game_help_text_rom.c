#include "portable/firered_portable_early_new_game_help_text_rom.h"

#include "portable/firered_portable_rom_text_family.h"

#include "characters.h"
#include "engine_internal.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const u8 gControlsGuide_Text_Intro[];
extern const u8 gControlsGuide_Text_DPad[];
extern const u8 gControlsGuide_Text_AButton[];
extern const u8 gControlsGuide_Text_BButton[];
extern const u8 gControlsGuide_Text_StartButton[];
extern const u8 gControlsGuide_Text_SelectButton[];
extern const u8 gControlsGuide_Text_LRButtons[];
extern const u8 gPikachuIntro_Text_Page1[];
extern const u8 gPikachuIntro_Text_Page2[];
extern const u8 gPikachuIntro_Text_Page3[];

static const char *const sEarlyNgEnvKeys[FIRERED_EARLY_NG_TX_COUNT] = {
    [FIRERED_EARLY_NG_TX_Intro] = "FIRERED_ROM_CTRL_GUIDE_TX_Intro",
    [FIRERED_EARLY_NG_TX_DPad] = "FIRERED_ROM_CTRL_GUIDE_TX_DPad",
    [FIRERED_EARLY_NG_TX_AButton] = "FIRERED_ROM_CTRL_GUIDE_TX_AButton",
    [FIRERED_EARLY_NG_TX_BButton] = "FIRERED_ROM_CTRL_GUIDE_TX_BButton",
    [FIRERED_EARLY_NG_TX_StartButton] = "FIRERED_ROM_CTRL_GUIDE_TX_StartButton",
    [FIRERED_EARLY_NG_TX_SelectButton] = "FIRERED_ROM_CTRL_GUIDE_TX_SelectButton",
    [FIRERED_EARLY_NG_TX_LRButtons] = "FIRERED_ROM_CTRL_GUIDE_TX_LRButtons",
    [FIRERED_EARLY_NG_TX_PikachuPage1] = "FIRERED_ROM_PIKACHU_INTRO_TX_Page1",
    [FIRERED_EARLY_NG_TX_PikachuPage2] = "FIRERED_ROM_PIKACHU_INTRO_TX_Page2",
    [FIRERED_EARLY_NG_TX_PikachuPage3] = "FIRERED_ROM_PIKACHU_INTRO_TX_Page3",
};

static const u8 *early_ng_get_fallback(unsigned entry_index, void *user)
{
    (void)user;

    switch ((FireredEarlyNewGameHelpTxId)entry_index)
    {
    case FIRERED_EARLY_NG_TX_Intro:
        return gControlsGuide_Text_Intro;
    case FIRERED_EARLY_NG_TX_DPad:
        return gControlsGuide_Text_DPad;
    case FIRERED_EARLY_NG_TX_AButton:
        return gControlsGuide_Text_AButton;
    case FIRERED_EARLY_NG_TX_BButton:
        return gControlsGuide_Text_BButton;
    case FIRERED_EARLY_NG_TX_StartButton:
        return gControlsGuide_Text_StartButton;
    case FIRERED_EARLY_NG_TX_SelectButton:
        return gControlsGuide_Text_SelectButton;
    case FIRERED_EARLY_NG_TX_LRButtons:
        return gControlsGuide_Text_LRButtons;
    case FIRERED_EARLY_NG_TX_PikachuPage1:
        return gPikachuIntro_Text_Page1;
    case FIRERED_EARLY_NG_TX_PikachuPage2:
        return gPikachuIntro_Text_Page2;
    case FIRERED_EARLY_NG_TX_PikachuPage3:
        return gPikachuIntro_Text_Page3;
    default:
        return gControlsGuide_Text_Intro;
    }
}

static const FireredRomTextFamilyParams sEarlyNgParams = {
    .trace_env_var = "FIRERED_TRACE_EARLY_NEW_GAME_HELP_ROM",
    .trace_prefix = "[firered early-ng-help-tx-rom]",
    .trace_bind_detail_env_var = "FIRERED_TRACE_EARLY_INTRO_FLOW",
    .entry_count = FIRERED_EARLY_NG_TX_COUNT,
    .env_key_names = sEarlyNgEnvKeys,
    .min_rom_size_for_scan = 0x200u,
    .env_offset_max_eos_search = 2048u,
    .scan_min_match_len = 12u,
    .get_fallback = early_ng_get_fallback,
    .try_profile_rom_off = NULL,
    .try_scan = NULL,
    .user = NULL,
    .trace_warn_multi_scan = TRUE,
};

static const u8 *sEarlyNgCache[FIRERED_EARLY_NG_TX_COUNT];
static u8 sEarlyNgBound;

extern char *getenv(const char *name);

void firered_portable_early_intro_flow_trace_snapshot(const char *where, FireredEarlyNewGameHelpTxId id, const u8 *text)
{
    static const char *const sIdNames[FIRERED_EARLY_NG_TX_COUNT] = {
        [FIRERED_EARLY_NG_TX_Intro] = "Intro",
        [FIRERED_EARLY_NG_TX_DPad] = "DPad",
        [FIRERED_EARLY_NG_TX_AButton] = "AButton",
        [FIRERED_EARLY_NG_TX_BButton] = "BButton",
        [FIRERED_EARLY_NG_TX_StartButton] = "StartButton",
        [FIRERED_EARLY_NG_TX_SelectButton] = "SelectButton",
        [FIRERED_EARLY_NG_TX_LRButtons] = "LRButtons",
        [FIRERED_EARLY_NG_TX_PikachuPage1] = "PikachuPage1",
        [FIRERED_EARLY_NG_TX_PikachuPage2] = "PikachuPage2",
        [FIRERED_EARLY_NG_TX_PikachuPage3] = "PikachuPage3",
    };
    const char *e;
    const u8 *rom;
    size_t rom_size;
    size_t j;

    if (where == NULL || text == NULL)
        return;
    e = getenv("FIRERED_TRACE_EARLY_INTRO_FLOW");
    if (e == NULL || e[0] == '\0' || strcmp(e, "0") == 0)
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;

    fprintf(stderr, "[firered early-intro-flow] print where=%s tx_id=%u (%s) ptr=%p", where, (unsigned)id,
        ((unsigned)id < (unsigned)FIRERED_EARLY_NG_TX_COUNT) ? sIdNames[id] : "?", (const void *)text);
    if (text >= rom && (size_t)(text - rom) < rom_size)
        fprintf(stderr, " rom_off=0x%zX", (size_t)(text - rom));
    else
        fprintf(stderr, " rom_off=(not-in-mapped-ROM)");
    fprintf(stderr, " head=");
    for (j = 0u; j < 32u; j++)
    {
        u8 b = text[j];

        fprintf(stderr, "%02X", b);
        if (b == EOS)
            break;
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

const u8 *firered_portable_early_new_game_help_get_text(FireredEarlyNewGameHelpTxId id)
{
    if ((unsigned)id >= (unsigned)FIRERED_EARLY_NG_TX_COUNT)
        return early_ng_get_fallback(FIRERED_EARLY_NG_TX_Intro, NULL);

    if (!sEarlyNgBound)
    {
        sEarlyNgBound = 1;
        firered_portable_rom_text_family_bind_all(&sEarlyNgParams, sEarlyNgCache);
    }

    return sEarlyNgCache[id];
}
