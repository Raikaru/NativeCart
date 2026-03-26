#include "global.h"
#include "gflib.h"
#include "random.h"
#include "overworld.h"
#include "constants/maps.h"
#include "load_save.h"
#include "item_menu.h"
#include "tm_case.h"
#include "berry_pouch.h"
#include "quest_log.h"
#include "wild_encounter.h"
#include "event_data.h"
#include "mail_data.h"
#include "play_time.h"
#include "money.h"
#include "battle_records.h"
#include "pokemon_size_record.h"
#include "pokemon_storage_system.h"
#include "roamer.h"
#include "item.h"
#include "player_pc.h"
#include "berry.h"
#include "easy_chat.h"
#include "union_room_chat.h"
#include "mystery_gift.h"
#include "renewable_hidden_items.h"
#include "trainer_tower.h"
#include "script.h"
#include "berry_powder.h"
#include "pokemon_jump.h"
#include "event_scripts.h"

#ifdef PORTABLE
extern void firered_runtime_trace_external(const char *message);

#ifndef NDEBUG
extern char *getenv(const char *name);

static int TraceNewGameEnvEnabled(void)
{
    static int s_init;
    static int s_on;
    const char *e;

    if (s_init)
        return s_on;
    s_init = 1;
    e = getenv("FIRERED_TRACE_NEW_GAME");
    s_on = (e != NULL && e[0] != '\0' && e[0] != '0');
    return s_on;
}
#else
static int TraceNewGameEnvEnabled(void)
{
    return 0;
}
#endif

static void TraceNewGame(const char *msg)
{
    if (!TraceNewGameEnvEnabled())
        return;
    firered_runtime_trace_external(msg);
}
#endif

// this file's functions
static void ResetMiniGamesResults(void);

// EWRAM vars
EWRAM_DATA bool8 gDifferentSaveFile = FALSE;

void SetTrainerId(u32 trainerId, u8 *dst)
{
    dst[0] = trainerId;
    dst[1] = trainerId >> 8;
    dst[2] = trainerId >> 16;
    dst[3] = trainerId >> 24;
}

void CopyTrainerId(u8 *dst, u8 *src)
{
    s32 i;
    for (i = 0; i < 4; i++)
        dst[i] = src[i];
}

static void InitPlayerTrainerId(void)
{
    u32 trainerId = (Random() << 0x10) | GetGeneratedTrainerIdLower();
    SetTrainerId(trainerId, gSaveBlock2Ptr->playerTrainerId);
}

static void SetDefaultOptions(void)
{
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_MID;
    gSaveBlock2Ptr->optionsWindowFrameType = 0;
    gSaveBlock2Ptr->optionsSound = OPTIONS_SOUND_MONO;
    gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
    gSaveBlock2Ptr->optionsBattleSceneOff = FALSE;
    gSaveBlock2Ptr->regionMapZoom = FALSE;
    gSaveBlock2Ptr->optionsButtonMode = OPTIONS_BUTTON_MODE_HELP;
}

static void ClearPokedexFlags(void)
{
    memset(&gSaveBlock2Ptr->pokedex.owned, 0, sizeof(gSaveBlock2Ptr->pokedex.owned));
    memset(&gSaveBlock2Ptr->pokedex.seen, 0, sizeof(gSaveBlock2Ptr->pokedex.seen));
}

static void ClearBattleTower(void)
{
    CpuFill32(0, &gSaveBlock2Ptr->battleTower, sizeof(gSaveBlock2Ptr->battleTower));
}

static void WarpToPlayersRoom(void)
{
    SetWarpDestination(MAP_GROUP(MAP_PALLET_TOWN_PLAYERS_HOUSE_2F), MAP_NUM(MAP_PALLET_TOWN_PLAYERS_HOUSE_2F), -1, 6, 6);
    WarpIntoMap();
}

void Sav2_ClearSetDefault(void)
{
    ClearSav2();
    SetDefaultOptions();
}

void ResetMenuAndMonGlobals(void)
{
    gDifferentSaveFile = FALSE;
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    ResetBagCursorPositions();
    ResetTMCaseCursorPos();
    BerryPouch_CursorResetToTop();
    ResetQuestLog();
    SeedWildEncounterRng(Random());
    ResetSpecialVars();
}

void NewGameInitData(void)
{
    u8 rivalName[PLAYER_NAME_LENGTH + 1];

#ifdef PORTABLE
    TraceNewGame("NewGameInitData: post-ResetQuestLog");
    TraceNewGame("NewGameInitData: pre-StringCopy rival");
#endif
    StringCopy(rivalName, gSaveBlock1Ptr->rivalName);
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: post-StringCopy rival");
#endif
    gDifferentSaveFile = TRUE;
    gSaveBlock2Ptr->encryptionKey = 0;
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    ClearBattleTower();
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: pre-ClearSav1");
#endif
    ClearSav1();
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: post-ClearSav1");
    TraceNewGame("NewGameInitData: pre-ClearMailData");
#endif
    ClearMailData();
    gSaveBlock2Ptr->specialSaveWarpFlags = 0;
    gSaveBlock2Ptr->gcnLinkFlags = 0;
    gSaveBlock2Ptr->unkFlag1 = TRUE;
    gSaveBlock2Ptr->unkFlag2 = FALSE;
    InitPlayerTrainerId();
    PlayTimeCounter_Reset();
    ClearPokedexFlags();
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: pre-InitEventData");
#endif
    InitEventData();
    ResetFameChecker();
    SetMoney(&gSaveBlock1Ptr->money, 3000);
    ResetGameStats();
    ClearPlayerLinkBattleRecords();
    InitHeracrossSizeRecord();
    InitMagikarpSizeRecord();
    EnableNationalPokedex_RSE();
    gPlayerPartyCount = 0;
    ZeroPlayerPartyMons();
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: pre-ResetPokemonStorageSystem");
#endif
    ResetPokemonStorageSystem();
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: post-ResetPokemonStorageSystem");
    TraceNewGame("NewGameInitData: pre-ClearRoamerData");
#endif
    ClearRoamerData();
    gSaveBlock1Ptr->registeredItem = 0;
    ClearBag();
    NewGameInitPCItems();
    ClearEnigmaBerries();
    InitEasyChatPhrases();
    ResetTrainerFanClub();
    UnionRoomChat_InitializeRegisteredTexts();
    ResetMiniGamesResults();
    ClearMysteryGift();
    SetAllRenewableItemFlags();
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: pre-WarpToPlayersRoom");
#endif
    WarpToPlayersRoom();
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: post-WarpToPlayersRoom");
    TraceNewGame("NewGameInitData: pre-RunScriptImmediately");
#endif
    RunScriptImmediately(EventScript_ResetAllMapFlags);
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: post-RunScriptImmediately");
    TraceNewGame("NewGameInitData: pre-StringCopy restore rival");
#endif
    StringCopy(gSaveBlock1Ptr->rivalName, rivalName);
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: post-StringCopy restore rival");
    TraceNewGame("NewGameInitData: pre-ResetTrainerTowerResults");
#endif
    ResetTrainerTowerResults();
#ifdef PORTABLE
    TraceNewGame("NewGameInitData: exit");
#endif
}

static void ResetMiniGamesResults(void)
{
    CpuFill16(0, &gSaveBlock2Ptr->berryCrush, sizeof(struct BerryCrush));
    SetBerryPowder(&gSaveBlock2Ptr->berryCrush.berryPowderAmount, 0);
    ResetPokemonJumpRecords();
    CpuFill16(0, &gSaveBlock2Ptr->berryPick, sizeof(struct BerryPickingResults));
}
