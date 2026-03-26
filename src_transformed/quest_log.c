#include "global.h"
#include "gflib.h"
#include "task.h"
#include "menu.h"
#include "menu_helpers.h"
#include "text_window.h"
#include "event_data.h"
#include "script.h"
#include "overworld.h"
#include "field_fadetransition.h"
#include "field_weather.h"
#include "event_object_movement.h"
#include "event_object_lock.h"
#include "field_door.h"
#include "field_player_avatar.h"
#include "metatile_behavior.h"
#include "item.h"
#include "region_map.h"
#include "map_name_popup.h"
#include "wild_encounter.h"
#include "help_system.h"
#include "pokemon_storage_system.h"
#include "save.h"
#include "quest_log_objects.h"
#include "quest_log_player.h"
#include "quest_log.h"
#include "new_menu_helpers.h"
#include "strings.h"
#include "constants/event_objects.h"
#include "constants/maps.h"
#include "constants/quest_log.h"
#include "constants/field_weather.h"
#include "constants/event_object_movement.h"
#include "constants/songs.h"

#ifdef PORTABLE
#include <stdio.h>
extern void firered_runtime_trace_external(const char *message);
extern char *getenv(const char *name);
#define firered_runtime_trace_external(...) ((void)0)

static int sQuestLogEnvCached;
static bool8 sQuestLogSkipPlaybackEnv;
static bool8 sQuestLogEnablePlaybackEnv;

static void QuestLogCachePortableEnvOnce(void)
{
    if (sQuestLogEnvCached)
        return;
    sQuestLogSkipPlaybackEnv = getenv("FIRERED_SKIP_QUEST_LOG_PLAYBACK") != NULL;
    sQuestLogEnablePlaybackEnv = getenv("FIRERED_ENABLE_QUEST_LOG_PLAYBACK") != NULL;
    sQuestLogEnvCached = 1;
}
#endif

enum {
    WIN_TOP_BAR,      // Contains the "Previously on..." text
    WIN_BOTTOM_BAR,   // Empty, only briefly visible at the end or when the description window isn't covering it.
    WIN_DESCRIPTION,
    WIN_COUNT
};

#define DESC_WIN_WIDTH (DISPLAY_WIDTH / 8)
#define DESC_WIN_HEIGHT 6
#define DESC_WIN_SIZE (DESC_WIN_WIDTH * DESC_WIN_HEIGHT * TILE_SIZE_4BPP)
#define QUEST_LOG_MOVEMENT_SCRIPT_SLOTS 256

// sQuestLogActionRecordBuffer should be large enough to fill a scene's script with the maximum number of actions
#define SCRIPT_BUFFER_SIZE (sizeof(gSaveBlock1Ptr->questLog[0].script) / sizeof(struct QuestLogAction))

enum {
    END_MODE_NONE,
    END_MODE_FINISH,
    END_MODE_SCENE,
};

struct PlaybackControl
{
    u8 state:4;
    u8 playingEvent:2;
    u8 endMode:2;
    u8 cursor;
    u8 timer;
    u8 overlapTimer;
};

struct FlagOrVarRecord
{
    u16 idx:15;
    u16 isFlag:1;
    u16 value;
};

COMMON_DATA u8 gQuestLogPlaybackState = 0;
COMMON_DATA u16 sMaxActionsInScene = 0;
COMMON_DATA struct FieldInput gQuestLogFieldInput = {0};
COMMON_DATA struct QuestLogAction * sCurSceneActions = NULL;

static struct FlagOrVarRecord * sFlagOrVarRecords;
static u16 sNumFlagsOrVars;

static EWRAM_DATA u8 sCurrentSceneNum = 0;
static EWRAM_DATA u8 sNumScenes = 0;
EWRAM_DATA u8 gQuestLogState = 0;
static EWRAM_DATA u16 sRecordSequenceStartIdx = 0;
static EWRAM_DATA u8 sWindowIds[WIN_COUNT] = {0};
EWRAM_DATA u16 *gQuestLogDefeatedWildMonRecord = NULL;
EWRAM_DATA u16 *gQuestLogRecordingPointer = NULL;
static EWRAM_DATA u16 *sEventData[32] = {NULL};
static EWRAM_DATA void (* sQuestLogCB)(void) = NULL;
static EWRAM_DATA u16 *sPalettesBackup = NULL;
static EWRAM_DATA struct PlaybackControl sPlaybackControl = {0};
static EWRAM_DATA struct QuestLogAction sQuestLogActionRecordBuffer[SCRIPT_BUFFER_SIZE] = {0};
EWRAM_DATA u16 gQuestLogCurActionIdx = 0;
// Playback queues scripts by recorded localId, not by template slot index.
static EWRAM_DATA u8 sMovementScripts[QUEST_LOG_MOVEMENT_SCRIPT_SLOTS][2] = {{0}};
static EWRAM_DATA u16 sNextActionDelay = 0;
static EWRAM_DATA u16 sLastQuestLogCursor = 0;
static EWRAM_DATA u16 sFlagOrVarPlayhead = 0;
static u8 sInvalidQuestLogSceneNum = 0xFF;
static const char *sInvalidQuestLogReason = NULL;
static u16 sInvalidQuestLogOffset = 0;
static u16 sInvalidQuestLogCommand = 0xFFFF;

static void QLogCB_Recording(void);
static void QLogCB_Playback(void);
static void SetPlayerInitialCoordsAtScene(u8);
static void SetNPCInitialCoordsAtScene(u8);
static void RecordSceneEnd(void);
static void BackUpTrainerRematches(void);
static void BackUpMapLayout(void);
static void SetGameStateAtScene(u8);
static u8 TryRecordActionSequence(struct QuestLogAction *);
static void Task_BeginQuestLogPlayback(u8);
static void QL_LoadObjectsAndTemplates(u8);
static void QLPlayback_InitOverworldState(void);
static bool8 QL_StartWarpExitStep(void);
static void Task_QuestLogWarpExitStep(u8 taskId);
static void SetPokemonCounts(void);
static u16 QuestLog_GetPartyCount(void);
static u16 QuestLog_GetBoxMonCount(void);
static void RestoreTrainerRematches(void);
static bool8 ReadQuestLogScriptFromSav1(u8, struct QuestLogAction *);
static void DoSceneEndTransition(s8 delay);
static void DoSkipToEndTransition(s8 delay);
static void QuestLog_AdvancePlayhead(void);
static void QuestLog_StartFinalScene(void);
static void RestorePlaybackCallbackIfStillActive(void);
static void Task_AvoidDisplay(u8);
static void QuestLog_PlayCurrentEvent(void);
static void HandleShowQuestLogMessage(void);
static u8 GetQuestLogTextDisplayDuration(void);
static void DrawSceneDescription(void);
static void CopyDescriptionWindowTiles(u8);
static void QuestLog_CloseTextWindow(void);
static void QuestLog_WaitFadeAndCancelPlayback(void);
static void EncodeQuestLogDisplayString(u8 *str);
static void SanitizeQuestLogDisplayString(u8 *str, u8 maxNewlines);
static bool8 FieldCB2_FinalScene(void);
static void Task_FinalScene_WaitFade(u8);
static void Task_QuestLogScene_SavedGame(u8);
static void Task_WaitAtEndOfQuestLog(u8);
static void Task_EndQuestLog(u8);
static bool8 RestoreScreenAfterPlayback(u8);
static void QL_SlightlyDarkenSomePals(void);
static void TogglePlaybackStateForOverworldLock(u8);
static void ResetActions(u8, struct QuestLogAction *, u16);
static bool8 RecordHeadAtEndOfEntryOrScriptContext2Enabled(void);
static bool8 RecordHeadAtEndOfEntry(void);
static bool8 InQuestLogDisabledLocation(void);
static bool8 TrySetLinkQuestLogEvent(u16, const u16 *);
static bool8 TrySetTrainerBattleQuestLogEvent(u16, const u16 *);
static bool8 IsQuestLogSceneValid(const struct QuestLogScene *scene);
static bool8 IsQuestLogScriptValid(u8 sceneNum);
static bool8 IsQuestLogScriptTailZeroed(u8 sceneNum, const u16 *script);
static void DiscardInvalidQuestLogAndContinue(u8 taskId);
static void AbortQuestLogPlaybackAndContinue(void);
static void NoteInvalidQuestLogData(u8 sceneNum, const char *reason);
static void NoteInvalidQuestLogScriptFailure(u8 sceneNum, const char *reason, const u16 *script);
static void TraceInvalidQuestLogData(const char *prefix);
static void WriteQuestLogStatusFile(const char *status, u8 sceneNum, const char *reason);
static void WriteQuestLogScriptDump(FILE *file, u8 sceneNum, u16 offset);
static void AppendQuestLogPlaybackTrace(const char *phase);
static void ResetQuestLogPlaybackTrace(void);
static void AppendQuestLogPlaybackEventTrace(const char *phase, const u16 *eventData);
static void AppendQuestLogPlaybackActionTrace(const char *phase, const struct QuestLogAction *action);
static void AppendQuestLogPlaybackTextTrace(const char *phase, const u8 *text);
static void DumpQuestLogSceneActions(const char *phase, const struct QuestLogAction *actions, u16 count);
#ifdef PORTABLE
static bool8 IsQuestLogPlaybackExplicitlyEnabled(void);
#endif

void QL_AppendPlaybackTrace(const char *phase);

static const struct WindowTemplate sWindowTemplates[WIN_COUNT] = {
    [WIN_TOP_BAR] = {
        .bg = 0,
        .tilemapLeft = 0, 
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x0e9
    },
    [WIN_BOTTOM_BAR] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 18,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x0ad
    },
    [WIN_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 14,
        .width = DESC_WIN_WIDTH,
        .height = DESC_WIN_HEIGHT,
        .paletteNum = 15,
        .baseBlock = 0x14c
    }
};

static const u8 sTextColors[3] = {TEXT_DYNAMIC_COLOR_6, TEXT_COLOR_WHITE, TEXT_DYNAMIC_COLOR_3};
static const u16 sDescriptionWindow_Gfx[] = INCBIN_U16("graphics/quest_log/description_window.4bpp");

static const u8 sQuestLogTextLineYCoords[] = {17, 10, 3};

static const u8 sQuestLogAsciiToGba[128] = {
    /* 0x00 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x08 */ 0x00, 0x00, CHAR_NEWLINE, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x10 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x18 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x20 */ CHAR_SPACE, CHAR_EXCL_MARK, 0x00, 0x00, 0x00, CHAR_PERCENT, CHAR_AMPERSAND, CHAR_SGL_QUOTE_RIGHT,
    /* 0x28 */ CHAR_LEFT_PAREN, CHAR_RIGHT_PAREN, 0x00, CHAR_PLUS, CHAR_COMMA, CHAR_HYPHEN, CHAR_PERIOD, CHAR_SLASH,
    /* 0x30 */ CHAR_0, CHAR_1, CHAR_2, CHAR_3, CHAR_4, CHAR_5, CHAR_6, CHAR_7,
    /* 0x38 */ CHAR_8, CHAR_9, CHAR_COLON, CHAR_SEMICOLON, CHAR_LESS_THAN, CHAR_EQUALS, CHAR_GREATER_THAN, CHAR_QUESTION_MARK,
    /* 0x40 */ 0x00, CHAR_A, CHAR_B, CHAR_C, CHAR_D, CHAR_E, CHAR_F, CHAR_G,
    /* 0x48 */ CHAR_H, CHAR_I, CHAR_J, CHAR_K, CHAR_L, CHAR_M, CHAR_N, CHAR_O,
    /* 0x50 */ CHAR_P, CHAR_Q, CHAR_R, CHAR_S, CHAR_T, CHAR_U, CHAR_V, CHAR_W,
    /* 0x58 */ CHAR_X, CHAR_Y, CHAR_Z, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x60 */ 0x00, CHAR_a, CHAR_b, CHAR_c, CHAR_d, CHAR_e, CHAR_f, CHAR_g,
    /* 0x68 */ CHAR_h, CHAR_i, CHAR_j, CHAR_k, CHAR_l, CHAR_m, CHAR_n, CHAR_o,
    /* 0x70 */ CHAR_p, CHAR_q, CHAR_r, CHAR_s, CHAR_t, CHAR_u, CHAR_v, CHAR_w,
    /* 0x78 */ CHAR_x, CHAR_y, CHAR_z, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void QL_AddASLROffset(void *oldSaveBlockPtr)
{
    // For some reason the caller passes the original pointer and the
    // amount the save moved is recalculated, instead of just passing
    // the offset to begin with.
    ptrdiff_t offset = (void *)gSaveBlock1Ptr - oldSaveBlockPtr;
    if (gQuestLogDefeatedWildMonRecord)
        gQuestLogDefeatedWildMonRecord = (void *)gQuestLogDefeatedWildMonRecord + offset;

    if (gQuestLogState == 0)
        return;

    if (gQuestLogRecordingPointer)
        gQuestLogRecordingPointer = (void *)gQuestLogRecordingPointer + offset;

    if (gQuestLogState == QL_STATE_PLAYBACK)
    {
        int i;
        for (i = 0; i < (int)ARRAY_COUNT(sEventData); i++)
            if (sEventData[i])
                sEventData[i] = (void *)sEventData[i] + offset;
    }
}

void ResetQuestLog(void)
{
    memset(gSaveBlock1Ptr->questLog, 0, sizeof(gSaveBlock1Ptr->questLog));
    sCurrentSceneNum = 0;
    gQuestLogState = 0;
    sQuestLogCB = NULL;
    gQuestLogRecordingPointer = NULL;
    gQuestLogDefeatedWildMonRecord = NULL;
    sInvalidQuestLogSceneNum = 0xFF;
    sInvalidQuestLogReason = NULL;
    sInvalidQuestLogOffset = 0;
    sInvalidQuestLogCommand = 0xFFFF;
    QL_ResetEventStates();
    ResetDeferredLinkEvent();
}

static void ClearSavedScene(u8 sceneNum)
{
    memset(&gSaveBlock1Ptr->questLog[sceneNum], 0, sizeof(gSaveBlock1Ptr->questLog[sceneNum]));
    gQuestLogDefeatedWildMonRecord = NULL;
}

void QL_ResetDefeatedWildMonRecord(void)
{
    gQuestLogDefeatedWildMonRecord = NULL;
}

void RunQuestLogCB(void)
{
#ifdef PORTABLE
    static bool8 sLoggedRunQuestLogCb;

    if (!sLoggedRunQuestLogCb)
    {
        char buffer[96];

        snprintf(buffer, sizeof(buffer), "RunQuestLogCB: enter cb=%p state=%u playback=%u",
                 sQuestLogCB, gQuestLogState, gQuestLogPlaybackState);
        firered_runtime_trace_external(buffer);
        sLoggedRunQuestLogCb = TRUE;
    }
    if (sQuestLogCB != NULL
     && gQuestLogPlaybackState == QL_PLAYBACK_STATE_STOPPED
     && gQuestLogState != 0
     && gQuestLogState != QL_STATE_RECORDING
     && gQuestLogState != QL_STATE_PLAYBACK
     && gQuestLogState != QL_STATE_PLAYBACK_LAST)
    {
        char buffer[96];

        snprintf(buffer, sizeof(buffer), "RunQuestLogCB: clearing invalid state=%u cb=%p",
                 gQuestLogState, sQuestLogCB);
        firered_runtime_trace_external(buffer);
        gQuestLogState = 0;
        sQuestLogCB = NULL;
    }
    if (sQuestLogCB != NULL && gQuestLogState == 0 && gQuestLogPlaybackState == QL_PLAYBACK_STATE_STOPPED)
    {
        char buffer[96];

        snprintf(buffer, sizeof(buffer), "RunQuestLogCB: clearing stale cb=%p", sQuestLogCB);
        firered_runtime_trace_external(buffer);
        sQuestLogCB = NULL;
    }
#endif
    AppendQuestLogPlaybackTrace("run_questlog_cb");
    if (sQuestLogCB != NULL)
        sQuestLogCB();
}

bool8 QL_IsRoomToSaveEvent(const void * cursor, size_t size)
{
    const void *start = gSaveBlock1Ptr->questLog[sCurrentSceneNum].script;
    const void *end = gSaveBlock1Ptr->questLog[sCurrentSceneNum].end;
    end -= size;
    if (cursor < start || cursor > end)
        return FALSE;
    return TRUE;
}

// Identical to QL_IsRoomToSaveEvent
bool8 QL_IsRoomToSaveAction(const void * cursor, size_t size)
{
    const void *start = gSaveBlock1Ptr->questLog[sCurrentSceneNum].script;
    const void *end = gSaveBlock1Ptr->questLog[sCurrentSceneNum].end;
    end -= size;
    if (cursor < start || cursor > end)
        return FALSE;
    return TRUE;
}

static void SetQuestLogState(u8 state)
{
    gQuestLogState = state;
    if (state == QL_STATE_RECORDING)
        sQuestLogCB = QLogCB_Recording;
    else
        sQuestLogCB = QLogCB_Playback;
}

static void QLogCB_Recording(void)
{
    if (TryRecordActionSequence(sQuestLogActionRecordBuffer) != 1)
    {
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        RecordSceneEnd();
        gQuestLogState = 0;
        sQuestLogCB = NULL;
    }
}

static void QLogCB_Playback(void)
{
    AppendQuestLogPlaybackTrace("qlogcb_playback_enter");
    if (sPlaybackControl.state == 2)
        sPlaybackControl.state = 0;

    if (sPlaybackControl.endMode == END_MODE_NONE)
    {
        if (gQuestLogPlaybackState != QL_PLAYBACK_STATE_STOPPED
         || sPlaybackControl.state == 1
         || (sPlaybackControl.cursor < ARRAY_COUNT(sEventData)
          && sEventData[sPlaybackControl.cursor] != NULL))
        {
            AppendQuestLogPlaybackTrace("qlogcb_playback_run_event");
            QuestLog_PlayCurrentEvent();
        }
        else
        {
            AppendQuestLogPlaybackTrace("qlogcb_playback_scene_end");
            sPlaybackControl.endMode = END_MODE_SCENE;
            LockPlayerFieldControls();
            DoSceneEndTransition(0);
        }
    }
}

void GetQuestLogState(void)
{
    gSpecialVar_Result = gQuestLogState;
}

u8 GetQuestLogStartType(void)
{
    return gSaveBlock1Ptr->questLog[sCurrentSceneNum].startType;
}

void QL_StartRecordingAction(u16 eventId)
{
    if (sCurrentSceneNum >= QUEST_LOG_SCENE_COUNT)
        sCurrentSceneNum = 0;

    ClearSavedScene(sCurrentSceneNum);
    QL_ResetRepeatEventTracker();
    gQuestLogRecordingPointer = gSaveBlock1Ptr->questLog[sCurrentSceneNum].script;
    if (IS_LINK_QL_EVENT(eventId) || eventId == QL_EVENT_DEPARTED)
        gSaveBlock1Ptr->questLog[sCurrentSceneNum].startType = QL_START_WARP;
    else
        gSaveBlock1Ptr->questLog[sCurrentSceneNum].startType = QL_START_NORMAL;
    SetPokemonCounts();
    SetPlayerInitialCoordsAtScene(sCurrentSceneNum);
    SetNPCInitialCoordsAtScene(sCurrentSceneNum);
    BackUpTrainerRematches();
    BackUpMapLayout();
    SetGameStateAtScene(sCurrentSceneNum);
    sRecordSequenceStartIdx = 0;
    ResetActions(QL_PLAYBACK_STATE_RECORDING, sQuestLogActionRecordBuffer, sizeof(sQuestLogActionRecordBuffer));
    TryRecordActionSequence(sQuestLogActionRecordBuffer);
    SetQuestLogState(QL_STATE_RECORDING);
}

static void SetPlayerInitialCoordsAtScene(u8 sceneNum)
{
    struct QuestLogScene * questLog = &gSaveBlock1Ptr->questLog[sceneNum];
    questLog->mapGroup = gSaveBlock1Ptr->location.mapGroup;
    questLog->mapNum = gSaveBlock1Ptr->location.mapNum;
    questLog->warpId = gSaveBlock1Ptr->location.warpId;
    questLog->x = gSaveBlock1Ptr->pos.x;
    questLog->y = gSaveBlock1Ptr->pos.y;
}

static void SetNPCInitialCoordsAtScene(u8 sceneNum)
{
    struct QuestLogScene * questLog = &gSaveBlock1Ptr->questLog[sceneNum];
    u16 i;

    QL_RecordObjects(questLog);

    for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
    {
        if (gSaveBlock1Ptr->objectEventTemplates[i].x < 0)
        {
            questLog->objectEventTemplates[i].x = -1 * gSaveBlock1Ptr->objectEventTemplates[i].x;
            questLog->objectEventTemplates[i].negx = TRUE;
        }
        else
        {
            questLog->objectEventTemplates[i].x = (u8)gSaveBlock1Ptr->objectEventTemplates[i].x;
            questLog->objectEventTemplates[i].negx = FALSE;
        }
        if (gSaveBlock1Ptr->objectEventTemplates[i].y < 0)
        {
            questLog->objectEventTemplates[i].y = (-gSaveBlock1Ptr->objectEventTemplates[i].y << 24) >> 24;
            questLog->objectEventTemplates[i].negy = TRUE;
        }
        else
        {
            questLog->objectEventTemplates[i].y = (u8)gSaveBlock1Ptr->objectEventTemplates[i].y;
            questLog->objectEventTemplates[i].negy = FALSE;
        }
        questLog->objectEventTemplates[i].elevation = gSaveBlock1Ptr->objectEventTemplates[i].objUnion.normal.elevation;
        questLog->objectEventTemplates[i].movementType = gSaveBlock1Ptr->objectEventTemplates[i].objUnion.normal.movementType;
    }
}

static void SetGameStateAtScene(u8 sceneNum)
{
    struct QuestLogScene * questLog = &gSaveBlock1Ptr->questLog[sceneNum];

    CpuCopy16(gSaveBlock1Ptr->flags, questLog->flags, sizeof(gSaveBlock1Ptr->flags));
    CpuCopy16(gSaveBlock1Ptr->vars, questLog->vars, sizeof(gSaveBlock1Ptr->vars));
}

static void BackUpTrainerRematches(void)
{
    u16 i, j;
    u16 vars[4];

    // Save the contents of gSaveBlock1Ptr->trainerRematches to the 4 saveblock
    // vars starting at VAR_QLBAK_TRAINER_REMATCHES. The rematch array is 100 bytes
    // long, but each byte is only ever 0 or 1 to indicate that a rematch is available.
    // They're compressed into single bits across 4 u16 save vars, which is only enough
    // to save 64 elements of gSaveBlock1Ptr->trainerRematches. 64 however is the maximum
    // that could ever be used, as its the maximum number of NPCs per map (OBJECT_EVENT_TEMPLATES_COUNT).
    for (i = 0; i < ARRAY_COUNT(vars); i++)
    {
        vars[i] = 0;

        // 16 bits per var
        for (j = 0; j < 16; j++)
        {
            if (gSaveBlock1Ptr->trainerRematches[16 * i + j])
                vars[i] += (1 << j);
        }
        VarSet(VAR_QLBAK_TRAINER_REMATCHES + i, vars[i]);
    }
}

static void BackUpMapLayout(void)
{
    VarSet(VAR_QLBAK_MAP_LAYOUT, gSaveBlock1Ptr->mapLayoutId);
}

static void RecordSceneEnd(void)
{
    QL_RecordAction_SceneEnd(gQuestLogRecordingPointer);
    if (++sCurrentSceneNum >= QUEST_LOG_SCENE_COUNT)
        sCurrentSceneNum = 0;
}

static bool8 TryRecordActionSequence(struct QuestLogAction * actions)
{
    u16 i;

    for (i = sRecordSequenceStartIdx; i < gQuestLogCurActionIdx; i++)
    {
        if (gQuestLogRecordingPointer == NULL)
            return FALSE;
        switch (actions[i].type)
        {
        case QL_ACTION_MOVEMENT:
        case QL_ACTION_GFX_CHANGE:
            gQuestLogRecordingPointer = QL_RecordAction_MovementOrGfxChange(gQuestLogRecordingPointer, &actions[i]);
            break;
        default:
            gQuestLogRecordingPointer = QL_RecordAction_Input(gQuestLogRecordingPointer, &actions[i]);
            break;
        }
        if (gQuestLogRecordingPointer == NULL)
        {
            gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
            return FALSE;
        }
    }

    if (gQuestLogPlaybackState == QL_PLAYBACK_STATE_STOPPED)
    {
        gQuestLogRecordingPointer = QL_RecordAction_SceneEnd(gQuestLogRecordingPointer);
        return FALSE;
    }
    sRecordSequenceStartIdx = gQuestLogCurActionIdx;
    return TRUE;
}

void TryStartQuestLogPlayback(u8 taskId)
{
    u8 i;
    bool8 foundInvalidScene = FALSE;

#ifdef PORTABLE
    if (!IsQuestLogPlaybackExplicitlyEnabled())
    {
        WriteQuestLogStatusFile("disabled", 0xFF, "quest log replay not explicitly enabled");
        SetMainCallback2(CB2_ContinueSavedGame);
        DestroyTask(taskId);
        return;
    }
#endif

    QL_EnableRecordingSteps();
    sNumScenes = 0;
    for (i = 0; i < QUEST_LOG_SCENE_COUNT; i++)
    {
        if (gSaveBlock1Ptr->questLog[i].startType != 0)
        {
            if (IsQuestLogSceneValid(&gSaveBlock1Ptr->questLog[i])
             && IsQuestLogScriptValid(i))
                sNumScenes++;
            else
                foundInvalidScene = TRUE;
        }
    }

#ifdef PORTABLE
    QuestLogCachePortableEnvOnce();
    if (sQuestLogSkipPlaybackEnv)
    {
        WriteQuestLogStatusFile("skipped", 0xFF, "quest log replay explicitly skipped");
        SetMainCallback2(CB2_ContinueSavedGame);
        DestroyTask(taskId);
        return;
    }
#endif

    if (foundInvalidScene)
    {
        DiscardInvalidQuestLogAndContinue(taskId);
    }
    else if (sNumScenes != 0)
    {
        ResetQuestLogPlaybackTrace();
        WriteQuestLogStatusFile("starting", 0xFF, "quest log replay passed startup validation");
        gHelpSystemEnabled = FALSE;
        Task_BeginQuestLogPlayback(taskId);
        DestroyTask(taskId);
    }
    else
    {
        WriteQuestLogStatusFile("empty", 0xFF, "quest log replay had no scenes");
        SetMainCallback2(CB2_ContinueSavedGame);
        DestroyTask(taskId);
    }
}

static bool8 IsQuestLogSceneValid(const struct QuestLogScene *scene)
{
    const struct MapHeader *header;

    if (scene == NULL)
    {
        NoteInvalidQuestLogData(0xFF, "scene pointer was NULL");
        return FALSE;
    }

    if (scene->startType != QL_START_NORMAL && scene->startType != QL_START_WARP)
    {
        NoteInvalidQuestLogData((u8)(scene - gSaveBlock1Ptr->questLog), "scene startType was invalid");
        return FALSE;
    }

    header = Overworld_GetMapHeaderByGroupAndId(scene->mapGroup, scene->mapNum);
    if (header == NULL || header->mapLayoutId == 0)
    {
        NoteInvalidQuestLogData((u8)(scene - gSaveBlock1Ptr->questLog), "scene map header/layout was invalid");
        return FALSE;
    }

    if (scene->x < -1 || scene->y < -1)
    {
        NoteInvalidQuestLogData((u8)(scene - gSaveBlock1Ptr->questLog), "scene coordinates were invalid");
        return FALSE;
    }

    return TRUE;
}

static bool8 IsQuestLogScriptValid(u8 sceneNum)
{
    u8 previousSceneNum = sCurrentSceneNum;
    struct QuestLogAction action;
    u16 *eventData;
    u16 *script;
    u16 *next;
    const u16 *scriptStart = gSaveBlock1Ptr->questLog[sceneNum].script;
    const u16 *scriptEnd = gSaveBlock1Ptr->questLog[sceneNum].end;
    u16 i;
    u16 actionCount = 0;
    u16 eventCount = 0;

    sCurrentSceneNum = sceneNum;
    script = gSaveBlock1Ptr->questLog[sceneNum].script;
    for (i = 0; i < ARRAY_COUNT(sEventData); i++)
    {
        if (script < scriptStart || script >= scriptEnd)
        {
            NoteInvalidQuestLogScriptFailure(sceneNum, "script pointer was out of bounds", script);
            sCurrentSceneNum = previousSceneNum;
            return FALSE;
        }
        if (IsQuestLogScriptTailZeroed(sceneNum, script))
        {
            sCurrentSceneNum = previousSceneNum;
            return TRUE;
        }

        switch (script[0] & QL_CMD_EVENT_MASK)
        {
        case QL_EVENT_INPUT:
            next = QL_LoadAction_Input(script, &action);
            actionCount++;
            break;
        case QL_EVENT_GFX_CHANGE:
        case QL_EVENT_MOVEMENT:
            next = QL_LoadAction_MovementOrGfxChange(script, &action);
            actionCount++;
            break;
        case QL_EVENT_SCENE_END:
            next = QL_LoadAction_SceneEnd(script, &action);
            actionCount++;
            break;
        case QL_EVENT_WAIT:
            next = QL_LoadAction_Wait(script, &action);
            actionCount++;
            break;
        default:
            next = QL_SkipCommand(script, &eventData);
            eventCount++;
            break;
        }

        if (actionCount > SCRIPT_BUFFER_SIZE || eventCount > ARRAY_COUNT(sEventData))
        {
            NoteInvalidQuestLogScriptFailure(sceneNum, "script exceeded action/event limits", script);
            sCurrentSceneNum = previousSceneNum;
            return FALSE;
        }
        if (next == NULL || next <= script || next > scriptEnd)
        {
            NoteInvalidQuestLogScriptFailure(sceneNum, "script command parse failed or advanced out of bounds", script);
            sCurrentSceneNum = previousSceneNum;
            return FALSE;
        }
        if ((script[0] & QL_CMD_EVENT_MASK) == QL_EVENT_SCENE_END)
        {
            sCurrentSceneNum = previousSceneNum;
            return TRUE;
        }

        script = next;
    }

    sCurrentSceneNum = previousSceneNum;
    return TRUE;
}

static bool8 IsQuestLogScriptTailZeroed(u8 sceneNum, const u16 *script)
{
    const u16 *scriptStart = gSaveBlock1Ptr->questLog[sceneNum].script;
    const u16 *scriptEnd = gSaveBlock1Ptr->questLog[sceneNum].end;

    if (script == NULL || script < scriptStart || script >= scriptEnd)
        return FALSE;

    while (script < scriptEnd)
    {
        if (*script != 0)
            return FALSE;
        script++;
    }

    return TRUE;
}

static void NoteInvalidQuestLogData(u8 sceneNum, const char *reason)
{
    sInvalidQuestLogSceneNum = sceneNum;
    sInvalidQuestLogReason = reason;
    sInvalidQuestLogOffset = 0;
    sInvalidQuestLogCommand = 0xFFFF;
}

static void NoteInvalidQuestLogScriptFailure(u8 sceneNum, const char *reason, const u16 *script)
{
    const u16 *scriptStart = gSaveBlock1Ptr->questLog[sceneNum].script;

    sInvalidQuestLogSceneNum = sceneNum;
    sInvalidQuestLogReason = reason;
    if (script != NULL && script >= scriptStart && script < gSaveBlock1Ptr->questLog[sceneNum].end)
    {
        sInvalidQuestLogOffset = (u16)(script - scriptStart);
        sInvalidQuestLogCommand = script[0] & QL_CMD_EVENT_MASK;
    }
    else
    {
        sInvalidQuestLogOffset = 0xFFFF;
        sInvalidQuestLogCommand = 0xFFFF;
    }
}

static void TraceInvalidQuestLogData(const char *prefix)
{
#ifdef PORTABLE
    char buffer[192];

    snprintf(
        buffer,
        sizeof(buffer),
        "%s scene=%u offset=%u command=%u reason=%s",
        prefix,
        sInvalidQuestLogSceneNum,
        sInvalidQuestLogOffset,
        sInvalidQuestLogCommand,
        (sInvalidQuestLogReason != NULL) ? sInvalidQuestLogReason : "unknown");
    firered_runtime_trace_external(buffer);
#else
    (void)prefix;
#endif
}

static void WriteQuestLogStatusFile(const char *status, u8 sceneNum, const char *reason)
{
#ifdef PORTABLE
    FILE *file = fopen("build/quest_log_replay_status.txt", "wb");

    if (file == NULL)
        return;

    fprintf(
        file,
        "status=%s\nscene=%u\noffset=%u\ncommand=%u\nreason=%s\n",
        status,
        sceneNum,
        sInvalidQuestLogOffset,
        sInvalidQuestLogCommand,
        (reason != NULL) ? reason : "unknown");
    WriteQuestLogScriptDump(file, sceneNum, sInvalidQuestLogOffset);
    fclose(file);
#else
    (void)status;
    (void)sceneNum;
    (void)reason;
#endif
}

static void WriteQuestLogScriptDump(FILE *file, u8 sceneNum, u16 offset)
{
#ifdef PORTABLE
    const struct QuestLogScene *scene;
    const u16 *script;
    const u16 *scriptEnd;
    u16 wordsToWrite;
    u16 i;

    if (file == NULL || sceneNum >= ARRAY_COUNT(gSaveBlock1Ptr->questLog))
        return;

    scene = &gSaveBlock1Ptr->questLog[sceneNum];
    script = scene->script;
    scriptEnd = scene->end;
    if (script == NULL || scriptEnd == NULL || script >= scriptEnd)
        return;

    fprintf(file, "script_words_total=%u\n", (unsigned)(scriptEnd - script));
    if (offset == 0xFFFF || offset >= (u16)(scriptEnd - script))
        return;

    fprintf(file, "raw_words=");
    wordsToWrite = (u16)(scriptEnd - (script + offset));
    if (wordsToWrite > 8)
        wordsToWrite = 8;

    for (i = 0; i < wordsToWrite; i++)
    {
        fprintf(file, "%s0x%04X", (i == 0) ? "" : ",", script[offset + i]);
    }
    fprintf(file, "\nraw_bytes=");
    for (i = 0; i < wordsToWrite; i++)
    {
        u8 low = script[offset + i] & 0xFF;
        u8 high = script[offset + i] >> 8;

        fprintf(file, "%s%02X,%02X", (i == 0) ? "" : ",", low, high);
    }
    fprintf(file, "\n");
#else
    (void)file;
    (void)sceneNum;
    (void)offset;
#endif
}

static void AppendQuestLogPlaybackTrace(const char *phase)
{
#ifdef PORTABLE
    FILE *file = fopen("build/quest_log_playback_trace.txt", "ab");

    if (file == NULL)
        return;

    fprintf(
        file,
        "phase=%s scene=%u playback=%u cursor=%u action=%u delay=%u endMode=%u state=%u\n",
        (phase != NULL) ? phase : "unknown",
        sCurrentSceneNum,
        gQuestLogPlaybackState,
        sPlaybackControl.cursor,
        gQuestLogCurActionIdx,
        sNextActionDelay,
        sPlaybackControl.endMode,
        sPlaybackControl.state);
    fclose(file);
#else
    (void)phase;
#endif
}

void QL_AppendPlaybackTrace(const char *phase)
{
    (void)phase;
}

static void ResetQuestLogPlaybackTrace(void)
{
}

static void AppendQuestLogPlaybackTaskTrace(const char *phase, s16 state, s16 timer)
{
    (void)phase;
    (void)state;
    (void)timer;
}

static void AppendQuestLogPlaybackEventTrace(const char *phase, const u16 *eventData)
{
    (void)phase;
    (void)eventData;
}

static void AppendQuestLogPlaybackActionTrace(const char *phase, const struct QuestLogAction *action)
{
    (void)phase;
    (void)action;
}

static void AppendQuestLogPlaybackTextTrace(const char *phase, const u8 *text)
{
    (void)phase;
    (void)text;
}

static void DumpQuestLogSceneActions(const char *phase, const struct QuestLogAction *actions, u16 count)
{
    (void)phase;
    (void)actions;
    (void)count;
}

static void DiscardInvalidQuestLogAndContinue(u8 taskId)
{
    TraceInvalidQuestLogData("QuestLog: invalid replay data detected, clearing quest log and continuing normally:");
    WriteQuestLogStatusFile("invalid", sInvalidQuestLogSceneNum, sInvalidQuestLogReason);
    ResetQuestLog();
    SetMainCallback2(CB2_ContinueSavedGame);
    DestroyTask(taskId);
}

static void AbortQuestLogPlaybackAndContinue(void)
{
    TraceInvalidQuestLogData("QuestLog: invalid replay script detected during playback setup, continuing normally:");
    WriteQuestLogStatusFile("invalid", sInvalidQuestLogSceneNum, sInvalidQuestLogReason);
    ResetQuestLog();
    gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
    sQuestLogCB = NULL;
    SetMainCallback2(CB2_ContinueSavedGame);
}

#ifdef PORTABLE
static bool8 IsQuestLogPlaybackExplicitlyEnabled(void)
{
    QuestLogCachePortableEnvOnce();
    return sQuestLogEnablePlaybackEnv;
}
#endif

static void Task_BeginQuestLogPlayback(u8 taskId)
{
    AppendQuestLogPlaybackTrace("begin_playback");
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_ROUTE1);
    gSaveBlock1Ptr->location.mapNum =  MAP_NUM(MAP_ROUTE1);
    gSaveBlock1Ptr->location.warpId = WARP_ID_NONE;
    sCurrentSceneNum = 0;
    gDisableMapMusicChangeOnMapLoad = 1;
    DisableWildEncounters(TRUE);
    QLPlayback_InitOverworldState();
}

void QL_InitSceneObjectsAndActions(void)
{
    if (!ReadQuestLogScriptFromSav1(sCurrentSceneNum, sQuestLogActionRecordBuffer))
    {
        AbortQuestLogPlaybackAndContinue();
        return;
    }
    DumpQuestLogSceneActions("scene_actions_loaded", sQuestLogActionRecordBuffer, ARRAY_COUNT(sQuestLogActionRecordBuffer));
    QL_ResetRepeatEventTracker();
    ResetActions(QL_PLAYBACK_STATE_RUNNING, sQuestLogActionRecordBuffer, sizeof(sQuestLogActionRecordBuffer));
    QL_LoadObjectsAndTemplates(sCurrentSceneNum);
}

static bool8 FieldCB2_QuestLogStartPlaybackWithWarpExit(void)
{
    AppendQuestLogPlaybackTrace("fieldcb_warp_exit_enter");
    LoadPalette(GetTextWindowPalette(4), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    SetQuestLogState(QL_STATE_PLAYBACK);
    FieldCB_DefaultWarpExit();
    sPlaybackControl = (struct PlaybackControl){};
    sPlaybackControl.state = 2;
    AppendQuestLogPlaybackTrace("fieldcb_warp_exit_ready");
    return 1;
}

static bool8 FieldCB2_QuestLogStartPlaybackStandingInPlace(void)
{
    AppendQuestLogPlaybackTrace("fieldcb_standing_enter");
    LoadPalette(GetTextWindowPalette(4), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    SetQuestLogState(QL_STATE_PLAYBACK);
    FieldCB_WarpExitFadeFromBlack();
    sPlaybackControl = (struct PlaybackControl){};
    sPlaybackControl.state = 2;
    AppendQuestLogPlaybackTrace("fieldcb_standing_ready");
    return 1;
}

void DrawPreviouslyOnQuestHeader(u8 sceneNum)
{
    u8 i;

    for (i = 0; i < WIN_COUNT; i++)
    {
        sWindowIds[i] = AddWindow(&sWindowTemplates[i]);
        FillWindowPixelRect(sWindowIds[i], 15, 0, 0, sWindowTemplates[i].width * 8, sWindowTemplates[i].height * 8);
    }

    StringExpandPlaceholders(gStringVar4, gText_QuestLog_PreviouslyOnYourQuest);

    // Scene numbers count from 4 to 0, 0 being where the player saved
    if (sceneNum != 0)
    {
        ConvertIntToDecimalStringN(gStringVar1, sceneNum, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringAppend(gStringVar4, gStringVar1);
    }

    EncodeQuestLogDisplayString(gStringVar4);
    SanitizeQuestLogDisplayString(gStringVar4, 0);
    AddTextPrinterParameterized4(sWindowIds[WIN_TOP_BAR], FONT_NORMAL, 2, 2, 1, 2, sTextColors, 0, gStringVar4);
    PutWindowTilemap(sWindowIds[WIN_TOP_BAR]);
    PutWindowTilemap(sWindowIds[WIN_BOTTOM_BAR]);
    CopyWindowToVram(sWindowIds[WIN_TOP_BAR], COPYWIN_GFX);
    CopyWindowToVram(sWindowIds[WIN_DESCRIPTION], COPYWIN_GFX);
    CopyWindowToVram(sWindowIds[WIN_BOTTOM_BAR], COPYWIN_FULL);
}

void CommitQuestLogWindow1(void)
{
    PutWindowTilemap(sWindowIds[WIN_BOTTOM_BAR]);
    CopyWindowToVram(sWindowIds[WIN_BOTTOM_BAR], COPYWIN_MAP);
}

static void QL_LoadObjectsAndTemplates(u8 sceneNum)
{
    struct QuestLogScene *questLog = &gSaveBlock1Ptr->questLog[sceneNum];
    u16 i;
    
    for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
    {
        if (questLog->objectEventTemplates[i].negx)
            gSaveBlock1Ptr->objectEventTemplates[i].x = -questLog->objectEventTemplates[i].x;
        else
            gSaveBlock1Ptr->objectEventTemplates[i].x = questLog->objectEventTemplates[i].x;
        if (questLog->objectEventTemplates[i].negy)
            gSaveBlock1Ptr->objectEventTemplates[i].y = -(u8)questLog->objectEventTemplates[i].y;
        else
            gSaveBlock1Ptr->objectEventTemplates[i].y = questLog->objectEventTemplates[i].y;
        gSaveBlock1Ptr->objectEventTemplates[i].objUnion.normal.elevation = questLog->objectEventTemplates[i].elevation;
        gSaveBlock1Ptr->objectEventTemplates[i].objUnion.normal.movementType = questLog->objectEventTemplates[i].movementType;
    }

    QL_LoadObjects(questLog, gSaveBlock1Ptr->objectEventTemplates);
}

static void QLPlayback_SetInitialPlayerPosition(u8 sceneNum, bool8 isWarp)
{
    if (!isWarp)
    {
        gSaveBlock1Ptr->location.mapGroup = gSaveBlock1Ptr->questLog[sceneNum].mapGroup;
        gSaveBlock1Ptr->location.mapNum = gSaveBlock1Ptr->questLog[sceneNum].mapNum;
        gSaveBlock1Ptr->location.warpId = gSaveBlock1Ptr->questLog[sceneNum].warpId;
        gSaveBlock1Ptr->pos.x = gSaveBlock1Ptr->questLog[sceneNum].x;
        gSaveBlock1Ptr->pos.y = gSaveBlock1Ptr->questLog[sceneNum].y;
    }
    else
    {
        struct WarpData warp;
        warp.mapGroup = gSaveBlock1Ptr->questLog[sceneNum].mapGroup;
        warp.mapNum = gSaveBlock1Ptr->questLog[sceneNum].mapNum;
        warp.warpId = gSaveBlock1Ptr->questLog[sceneNum].warpId;
        warp.x = gSaveBlock1Ptr->questLog[sceneNum].x;
        warp.y = gSaveBlock1Ptr->questLog[sceneNum].y;
        Overworld_SetWarpDestinationFromWarp(&warp);
    }
}

static void QLPlayback_InitOverworldState(void)
{
    AppendQuestLogPlaybackTrace("init_overworld_state");
    gQuestLogState = QL_STATE_PLAYBACK;
    ResetSpecialVars();
    ClearBag();
    ClearPCItemSlots();
    if (GetQuestLogStartType() == QL_START_NORMAL)
    {
        QLPlayback_SetInitialPlayerPosition(sCurrentSceneNum, FALSE);
        gFieldCallback2 = FieldCB2_QuestLogStartPlaybackStandingInPlace;
        SetMainCallback2(CB2_SetUpOverworldForQLPlayback);
    }
    else
    {
        QLPlayback_SetInitialPlayerPosition(sCurrentSceneNum, TRUE);
        WarpIntoMap();
        gFieldCallback2 = FieldCB2_QuestLogStartPlaybackWithWarpExit;
        SetMainCallback2(CB2_SetUpOverworldForQLPlaybackWithWarpExit);
    }
}

static bool8 QL_StartWarpExitStep(void)
{
    if (GetQuestLogStartType() != QL_START_WARP)
        return FALSE;

    CreateTask(Task_QuestLogWarpExitStep, 10);
    sQuestLogCB = NULL;
    return TRUE;
}

static void Task_QuestLogWarpExitStep(u8 taskId)
{
    struct ObjectEvent *playerObject = &gObjectEvents[gPlayerAvatar.objectEventId];
    s16 *x = &gTasks[taskId].data[2];
    s16 *y = &gTasks[taskId].data[3];
    s16 *doorTaskId = &gTasks[taskId].data[4];
    s16 *warpMode = &gTasks[taskId].data[5];
    bool8 isDoor = (*warpMode == 1);
    u16 metatileBehavior;

    AppendQuestLogPlaybackTaskTrace("ql_warp_exit_task", gTasks[taskId].data[0], gTasks[taskId].data[1]);

    switch (gTasks[taskId].data[0])
    {
    case 0:
        FreezeObjectEvents();
        LockPlayerFieldControls();
        PlayerGetDestCoords(x, y);
        *doorTaskId = -1;
        metatileBehavior = MapGridGetMetatileBehaviorAt(*x, *y);
        if (MetatileBehavior_IsWarpDoor_2(metatileBehavior) || MetatileBehavior_IsNonAnimDoor(metatileBehavior))
            *warpMode = 1;
        else if (MetatileBehavior_IsDirectionalStairWarp(metatileBehavior))
            *warpMode = 2;
        else
        {
            metatileBehavior = MapGridGetMetatileBehaviorAt(playerObject->currentCoords.x, playerObject->currentCoords.y);
            if (MetatileBehavior_IsWarpDoor_2(metatileBehavior) || MetatileBehavior_IsNonAnimDoor(metatileBehavior))
                *warpMode = 1;
            else if (MetatileBehavior_IsDirectionalStairWarp(metatileBehavior))
                *warpMode = 2;
            else
                *warpMode = 0;
        }

        isDoor = (*warpMode == 1);
        if (isDoor)
            SetPlayerInvisibility(TRUE);
        gTasks[taskId].data[0] = 1;
        break;
    case 1:
        isDoor = (*warpMode == 1);
        if (!gPaletteFade.active)
        {
            if (isDoor)
            {
                SetPlayerInvisibility(FALSE);
                PlaySE(GetDoorSoundEffect(*x, *y));
                *doorTaskId = FieldAnimateDoorOpen(*x, *y);
                gTasks[taskId].data[0] = 2;
                break;
            }
            if (!ObjectEventSetHeldMovement(playerObject, GetWalkNormalMovementAction(GetPlayerFacingDirection())))
            {
                UnfreezeObjectEvents();
                UnlockPlayerFieldControls();
                DestroyTask(taskId);
                break;
            }
            gTasks[taskId].data[0] = 3;
        }
        break;
    case 2:
        isDoor = (*warpMode == 1);
        if (*doorTaskId < 0 || gTasks[*doorTaskId].isActive != TRUE)
        {
            if (!ObjectEventSetHeldMovement(playerObject, GetWalkNormalMovementAction(GetPlayerFacingDirection())))
            {
                UnfreezeObjectEvents();
                UnlockPlayerFieldControls();
                DestroyTask(taskId);
                break;
            }
            gTasks[taskId].data[0] = 3;
        }
        break;
    case 3:
        isDoor = (*warpMode == 1);
        if (walkrun_is_standing_still())
        {
            ObjectEventClearHeldMovementIfFinished(playerObject);
            if (isDoor)
            {
                *doorTaskId = FieldAnimateDoorClose(*x, *y);
                gTasks[taskId].data[0] = 4;
                break;
            }
            UnfreezeObjectEvents();
            UnlockPlayerFieldControls();
            RestorePlaybackCallbackIfStillActive();
            DestroyTask(taskId);
        }
        break;
    case 4:
        if (*doorTaskId < 0 || gTasks[*doorTaskId].isActive != TRUE)
        {
            UnfreezeObjectEvents();
            UnlockPlayerFieldControls();
            RestorePlaybackCallbackIfStillActive();
            DestroyTask(taskId);
        }
        break;
    }
}

void QL_CopySaveState(void)
{
    struct QuestLogScene * questLog = &gSaveBlock1Ptr->questLog[sCurrentSceneNum];

    CpuCopy16(questLog->flags, gSaveBlock1Ptr->flags, sizeof(gSaveBlock1Ptr->flags));
    CpuCopy16(questLog->vars, gSaveBlock1Ptr->vars, sizeof(gSaveBlock1Ptr->vars));
    RestoreTrainerRematches();
}

// The number of bits allocated to store the number of pokemon in the PC
#define NUM_PC_COUNT_BITS  12

void QL_ResetPartyAndPC(void)
{
    struct {
        struct Pokemon mon;
        u16 partyCount;
        u16 boxMonCount;
    } *prev = AllocZeroed(sizeof(*prev));
    u16 packedCounts, i, count, j;

    CreateMon(&prev->mon, SPECIES_RATTATA, 1, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
    packedCounts = VarGet(VAR_QUEST_LOG_MON_COUNTS);
    prev->partyCount = packedCounts >> NUM_PC_COUNT_BITS;
    prev->boxMonCount = packedCounts % (1 << NUM_PC_COUNT_BITS);

    count = QuestLog_GetPartyCount();
    if (count > prev->partyCount)
    {
        for (i = 0; i < count - prev->partyCount; i++)
            ZeroMonData(&gPlayerParty[PARTY_SIZE - 1 - i]);
    }
    else if (count < prev->partyCount)
    {
        // Clear 5 slots in the PC?
        for (i = 0; i < PARTY_SIZE - 1; i++)
            ZeroBoxMonAt(0, i);

        // Replace the additional slots with placeholder Pokémon.
        for (i = count; i < prev->partyCount; i++)
            CopyMon(&gPlayerParty[i], &prev->mon, sizeof(struct Pokemon));
    }

    count = QuestLog_GetBoxMonCount();
    if (count > prev->boxMonCount)
    {
        for (i = 0; i < TOTAL_BOXES_COUNT; i++)
        {
            for (j = 0; j < IN_BOX_COUNT; j++)
            {
                if (GetBoxMonDataAt(i, j, MON_DATA_SANITY_HAS_SPECIES))
                {
                    ZeroBoxMonAt(i, j);
                    if (--count == prev->boxMonCount)
                        break;
                }
            }
            if (count == prev->boxMonCount)
                break;
        }
    }
    else if (count < prev->boxMonCount)
    {
        for (i = 0; i < TOTAL_BOXES_COUNT; i++)
        {
            for (j = 0; j < IN_BOX_COUNT; j++)
            {
                struct BoxPokemon * boxMon = GetBoxedMonPtr(i, j);
                if (!GetBoxMonData(boxMon, MON_DATA_SANITY_HAS_SPECIES))
                {
                    // Replace the additional slots with placeholder Pokémon.
                    CopyMon(boxMon, &prev->mon.box, sizeof(struct BoxPokemon));
                    if (++count == prev->boxMonCount)
                        break;
                }
            }
            if (count == prev->boxMonCount)
                break;
        }
    }
    Free(prev);
}

static void SetPokemonCounts(void)
{
    u16 partyCount = QuestLog_GetPartyCount();
    u16 boxesCount = QuestLog_GetBoxMonCount();
    VarSet(VAR_QUEST_LOG_MON_COUNTS, (partyCount << NUM_PC_COUNT_BITS) + boxesCount);
}

static u16 QuestLog_GetPartyCount(void)
{
    u16 count = 0;
    u16 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SANITY_HAS_SPECIES))
            count++;
    }

    return count;
}

static u16 QuestLog_GetBoxMonCount(void)
{
    u16 count = 0;
    u16 i, j;

    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
    {
        for (j = 0; j < IN_BOX_COUNT; j++)
        {
            if (GetBoxMonDataAt(i, j, MON_DATA_SANITY_HAS_SPECIES))
                count++;
        }
    }

    return count;
}

// Inverse of BackUpTrainerRematches
static void RestoreTrainerRematches(void)
{
    u16 i, j;
    u16 vars[4];

    for (i = 0; i < ARRAY_COUNT(vars); i++)
    {
        vars[i] = VarGet(VAR_QLBAK_TRAINER_REMATCHES + i);

        // 16 bits per var
        for (j = 0; j < 16; j++)
        {
            if (vars[i] & 1)
                gSaveBlock1Ptr->trainerRematches[16 * i + j] = 30;
            else
                gSaveBlock1Ptr->trainerRematches[16 * i + j] = 0;
            vars[i] >>= 1;
        }
    }
}

// Inverse of BackUpMapLayout
void QL_RestoreMapLayoutId(void)
{
    gSaveBlock1Ptr->mapLayoutId = VarGet(VAR_QLBAK_MAP_LAYOUT);
    if (gSaveBlock1Ptr->mapLayoutId == 0)
    {
        struct MapHeader header = *Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);
        gSaveBlock1Ptr->mapLayoutId = header.mapLayoutId;
    }
}

static bool8 ReadQuestLogScriptFromSav1(u8 sceneNum, struct QuestLogAction * actions)
{
    u8 previousSceneNum = sCurrentSceneNum;
    u16 i;
    u16 *currentScript;
    u16 *script;
    const u16 *scriptStart = gSaveBlock1Ptr->questLog[sceneNum].script;
    const u16 *scriptEnd = gSaveBlock1Ptr->questLog[sceneNum].end;
    u16 actionNum = 0;
    u16 eventNum = 0;

    memset(actions, 0, ARRAY_COUNT(sEventData) * sizeof(struct QuestLogAction));
    for (i = 0; i < ARRAY_COUNT(sEventData); i++)
        sEventData[i] = NULL;

    sCurrentSceneNum = sceneNum;
    script = gSaveBlock1Ptr->questLog[sceneNum].script;
    for (i = 0; i < ARRAY_COUNT(sEventData); i++)
    {
        currentScript = script;
        if (script < scriptStart || script >= scriptEnd)
        {
            NoteInvalidQuestLogScriptFailure(sceneNum, "live script read started out of bounds", script);
            sCurrentSceneNum = previousSceneNum;
            return FALSE;
        }
        if (IsQuestLogScriptTailZeroed(sceneNum, script))
        {
            sCurrentSceneNum = previousSceneNum;
            return TRUE;
        }

        switch (script[0] & QL_CMD_EVENT_MASK)
        {
        case QL_EVENT_INPUT:
            if (actionNum >= ARRAY_COUNT(sQuestLogActionRecordBuffer))
            {
                NoteInvalidQuestLogScriptFailure(sceneNum, "live script read overflowed action buffer", script);
                sCurrentSceneNum = previousSceneNum;
                return FALSE;
            }
            script = QL_LoadAction_Input(script, &actions[actionNum]);
            actionNum++;
            break;
        case QL_EVENT_GFX_CHANGE:
        case QL_EVENT_MOVEMENT:
            if (actionNum >= ARRAY_COUNT(sQuestLogActionRecordBuffer))
            {
                NoteInvalidQuestLogScriptFailure(sceneNum, "live script read overflowed action buffer", script);
                sCurrentSceneNum = previousSceneNum;
                return FALSE;
            }
            script = QL_LoadAction_MovementOrGfxChange(script, &actions[actionNum]);
            actionNum++;
            break;
        case QL_EVENT_SCENE_END:
            if (actionNum >= ARRAY_COUNT(sQuestLogActionRecordBuffer))
            {
                NoteInvalidQuestLogScriptFailure(sceneNum, "live script read overflowed action buffer", script);
                sCurrentSceneNum = previousSceneNum;
                return FALSE;
            }
            script = QL_LoadAction_SceneEnd(script, &actions[actionNum]);
            if (script == NULL)
            {
                NoteInvalidQuestLogScriptFailure(sceneNum, "live scene-end command parse failed", currentScript);
                sCurrentSceneNum = previousSceneNum;
                return FALSE;
            }
            sCurrentSceneNum = previousSceneNum;
            return script != NULL;
        case QL_EVENT_WAIT:
            if (actionNum >= ARRAY_COUNT(sQuestLogActionRecordBuffer))
            {
                NoteInvalidQuestLogScriptFailure(sceneNum, "live script read overflowed action buffer", script);
                sCurrentSceneNum = previousSceneNum;
                return FALSE;
            }
            script = QL_LoadAction_Wait(script, &actions[actionNum]);
            actionNum++;
            break;
        default: // Normal event
            if (eventNum >= ARRAY_COUNT(sEventData))
            {
                NoteInvalidQuestLogScriptFailure(sceneNum, "live script read overflowed event buffer", script);
                sCurrentSceneNum = previousSceneNum;
                return FALSE;
            }
            script = QL_SkipCommand(script, &sEventData[eventNum]);
            if (eventNum == 0)
                QL_UpdateLastDepartedLocation(sEventData[0]);
            eventNum++;
            break;
        }
        if (script == NULL)
        {
            NoteInvalidQuestLogScriptFailure(sceneNum, "live script command parse returned NULL", currentScript);
            sCurrentSceneNum = previousSceneNum;
            return FALSE;
        }
        if (script <= scriptStart || script > scriptEnd)
        {
            NoteInvalidQuestLogScriptFailure(sceneNum, "live script command advanced out of bounds", script);
            sCurrentSceneNum = previousSceneNum;
            return FALSE;
        }
    }

    sCurrentSceneNum = previousSceneNum;
    return TRUE;
}

static void DoSceneEndTransition(s8 delay)
{
    FadeScreen(FADE_TO_BLACK, delay);
    sQuestLogCB = QuestLog_AdvancePlayhead;
}

static void QuestLog_AdvancePlayhead(void)
{
    if (gPaletteFade.active)
        return;

    LockPlayerFieldControls();
    if (++sCurrentSceneNum < QUEST_LOG_SCENE_COUNT && gSaveBlock1Ptr->questLog[sCurrentSceneNum].startType != 0)
    {
        sNumScenes--;
        QLPlayback_InitOverworldState();
    }
    else
    {
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        QuestLog_StartFinalScene();
    }
}

static void QuestLog_StartFinalScene(void)
{
    ResetSpecialVars();
    Save_ResetSaveCounters();
    LoadGameSave(SAVE_NORMAL);
    SetMainCallback2(CB2_EnterFieldFromQuestLog);
    gFieldCallback2 = FieldCB2_FinalScene;
    FreeAllWindowBuffers();
    gQuestLogState = QL_STATE_PLAYBACK_LAST;
    sQuestLogCB = NULL;
}

static void RestorePlaybackCallbackIfStillActive(void)
{
    if (gQuestLogState == QL_STATE_PLAYBACK && gQuestLogPlaybackState != QL_PLAYBACK_STATE_STOPPED)
        sQuestLogCB = QLogCB_Playback;
}

void QuestLog_AdvancePlayhead_(void)
{
    QuestLog_AdvancePlayhead();
}

#define tTimer data[0]
#define tState data[1]
#define DATA_IDX_CALLBACK 14 // data[14] and data[15]

// This is used to avoid recording or displaying certain windows or images, like a shop menu.
// During playback it returns TRUE (meaning the action should be avoided) and calls the
// provided callback, which would be used to e.g. destroy any resources that were set up to do
// whatever is being avoided. In all cases the provided callback will be QL_DestroyAbortedDisplay.
// If we are not currently in playback return FALSE (meaning allow the action to occur) and
// stop recording (if we are currently).
bool8 QL_AvoidDisplay(void (*callback)(void))
{
    u8 taskId;

    switch (gQuestLogState)
    {
    case QL_STATE_RECORDING:
        QuestLog_CutRecording();
        break;
    case QL_STATE_PLAYBACK:
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_ACTION_END;
        taskId = CreateTask(Task_AvoidDisplay, 80);
        gTasks[taskId].tTimer = 0;
        gTasks[taskId].tState = 0;
        SetWordTaskArg(taskId, DATA_IDX_CALLBACK, (uintptr_t)callback);
        return TRUE;
    }
    return FALSE;
}

static void Task_AvoidDisplay(u8 taskId)
{
    void (*routine)(void);
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        // Instead of displaying anything, wait and then end the scene.
        if (++tTimer == 127)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, 0);
            sPlaybackControl.endMode = END_MODE_SCENE;
            tState++;
        }
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
            
            // Call the provided function (if any). In practice this is always QL_DestroyAbortedDisplay
            routine = (void (*)(void)) GetWordTaskArg(taskId, DATA_IDX_CALLBACK);
            if (routine != NULL)
                routine();

            DestroyTask(taskId);
            sQuestLogCB = QuestLog_AdvancePlayhead;
        }
        break;
    }
}

#undef tTimer
#undef tState

static void QuestLog_PlayCurrentEvent(void)
{
    if (sPlaybackControl.state == 1)
    {
        if (--sPlaybackControl.timer != 0)
            return;
        sPlaybackControl.state = 0;
        sPlaybackControl.playingEvent = TRUE;
        TogglePlaybackStateForOverworldLock(2);
    }

    if (sPlaybackControl.playingEvent == TRUE)
    {
        if (++sPlaybackControl.overlapTimer > 15)
        {
            QuestLog_CloseTextWindow();
            sPlaybackControl.playingEvent = FALSE;
            sPlaybackControl.overlapTimer = 0;
        }
    }
    if (sPlaybackControl.cursor < ARRAY_COUNT(sEventData))
    {
        if (QL_TryRepeatEvent(sEventData[sPlaybackControl.cursor]) == TRUE)
        {
            AppendQuestLogPlaybackEventTrace("qlogcb_playback_repeat_event", sEventData[sPlaybackControl.cursor]);
            HandleShowQuestLogMessage();
        }
        else if (QL_LoadEvent(sEventData[sPlaybackControl.cursor]) == TRUE)
        {
            AppendQuestLogPlaybackEventTrace("qlogcb_playback_load_event", sEventData[sPlaybackControl.cursor]);
            HandleShowQuestLogMessage();
        }
        else
        {
            AppendQuestLogPlaybackEventTrace("qlogcb_playback_event_blocked", sEventData[sPlaybackControl.cursor]);
        }
    }
}

static void HandleShowQuestLogMessage(void)
{
    if (sPlaybackControl.state == 0)
    {
        AppendQuestLogPlaybackTextTrace("show_message_before_draw", gStringVar4);
        sPlaybackControl.state = 1;
        sPlaybackControl.playingEvent = FALSE;
        sPlaybackControl.overlapTimer = 0;
        sPlaybackControl.timer = GetQuestLogTextDisplayDuration();
        if (gQuestLogRepeatEventTracker.counter == 0)
            sPlaybackControl.cursor++;
        if (sPlaybackControl.cursor > ARRAY_COUNT(sEventData))
            return;
        DrawSceneDescription();
        AppendQuestLogPlaybackTextTrace("show_message_after_draw", gStringVar4);
    }
    TogglePlaybackStateForOverworldLock(1); // lock
}

static u8 GetQuestLogTextDisplayDuration(void)
{
    u16 i;
    u16 count = 0;

    for (i = 0; i < 0x400 && gStringVar4[i] != EOS; i++)
    {
        if (gStringVar4[i] != CHAR_NEWLINE)
            count++;
    }

    if (count < 20)
        return 0x5F;
    if (count < 36)
        return 0x7F;
    if (count < 46)
        return 0xBF;
    return 0xFF;
}

bool8 QL_IsTrainerSightDisabled(void)
{
    if (gQuestLogState != QL_STATE_PLAYBACK)
        return FALSE;
    if (gQuestLogPlaybackState == QL_PLAYBACK_STATE_STOPPED || sPlaybackControl.state == 1 || sPlaybackControl.state == 2)
        return TRUE;
    return FALSE;
}

void QL_HandleInput(void)
{
    // Ignore input if we're currently ending a scene/playback
    if (sPlaybackControl.endMode != END_MODE_NONE)
        return;

    if (JOY_NEW(A_BUTTON))
    {
        // Pressed A, skip to next scene
        sPlaybackControl.endMode = END_MODE_SCENE;
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        DoSceneEndTransition(-3);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        // Pressed B, end playback
        sPlaybackControl.endMode = END_MODE_FINISH;
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        DoSkipToEndTransition(-3);
    }
}

bool8 QuestLogScenePlaybackIsEnding(void)
{
    if (sPlaybackControl.endMode != END_MODE_NONE)
        return TRUE;
    return FALSE;
}

void QuestLog_DrawPreviouslyOnQuestHeaderIfInPlaybackMode(void)
{
    if (gQuestLogState == QL_STATE_PLAYBACK)
        DrawPreviouslyOnQuestHeader(sNumScenes);
}

static void DrawSceneDescription(void)
{
    u16 i;
    u8 numLines = 0;

    AppendQuestLogPlaybackTextTrace("draw_scene_description_enter", gStringVar4);

    // Quest Log windows are laid out for at most three display lines.
    // Saved replay strings should not carry prompt/scroll controls here, but
    // if they do, strip them before rendering so they can't corrupt nearby
    // replay window buffers.
    EncodeQuestLogDisplayString(gStringVar4);
    SanitizeQuestLogDisplayString(gStringVar4, ARRAY_COUNT(sQuestLogTextLineYCoords) - 1);

    for (i = 0; i < 0x100 && gStringVar4[i] != EOS; i++)
    {
        if (gStringVar4[i] == CHAR_NEWLINE)
            numLines++;
    }
    if (numLines >= ARRAY_COUNT(sQuestLogTextLineYCoords))
        numLines = ARRAY_COUNT(sQuestLogTextLineYCoords) - 1;

    PutWindowTilemap(sWindowIds[WIN_DESCRIPTION]);
    CopyDescriptionWindowTiles(sWindowIds[WIN_DESCRIPTION]);
    AddTextPrinterParameterized4(sWindowIds[WIN_DESCRIPTION], FONT_NORMAL, 2, sQuestLogTextLineYCoords[numLines], 1, 0, sTextColors, 0, gStringVar4);
    // WIN_DESCRIPTION occupies rows 14-19 and overlaps WIN_BOTTOM_BAR on
    // rows 18-19, so re-apply the bottom bar tilemap after drawing the
    // description window.
    PutWindowTilemap(sWindowIds[WIN_BOTTOM_BAR]);
    ScheduleBgCopyTilemapToVram(0);
    AppendQuestLogPlaybackTextTrace("draw_scene_description_done", gStringVar4);
}

static void EncodeQuestLogDisplayString(u8 *str)
{
    u16 src = 0;
    u16 dst = 0;
    u8 encoded[0x400];

    if (str == NULL)
        return;

    while (str[src] != EOS && str[src] != '\0' && dst + 1 < ARRAY_COUNT(encoded))
    {
        if (str[src] == 0xC3 && str[src + 1] == 0xA9)
        {
            encoded[dst++] = CHAR_E_ACUTE;
            src += 2;
            continue;
        }
        if (str[src] == 0xE2 && str[src + 1] == 0x80)
        {
            switch (str[src + 2])
            {
            case 0xA6:
                encoded[dst++] = CHAR_ELLIPSIS;
                src += 3;
                continue;
            case 0x98:
                encoded[dst++] = CHAR_SGL_QUOTE_LEFT;
                src += 3;
                continue;
            case 0x99:
                encoded[dst++] = CHAR_SGL_QUOTE_RIGHT;
                src += 3;
                continue;
            case 0x9C:
                encoded[dst++] = CHAR_DBL_QUOTE_LEFT;
                src += 3;
                continue;
            case 0x9D:
                encoded[dst++] = CHAR_DBL_QUOTE_RIGHT;
                src += 3;
                continue;
            }
        }

        if (str[src] < 0x80)
            encoded[dst++] = sQuestLogAsciiToGba[str[src++]];
        else
            encoded[dst++] = str[src++];
    }

    encoded[dst] = EOS;
    memcpy(str, encoded, dst + 1);
}

static void SanitizeQuestLogDisplayString(u8 *str, u8 maxNewlines)
{
    u16 src = 0;
    u16 dst = 0;
    u8 newlineCount = 0;

    if (str == NULL)
        return;

    while (str[src] != EOS && dst < 0xFF)
    {
        u8 c = str[src++];

        switch (c)
        {
        case CHAR_PROMPT_SCROLL:
        case CHAR_PROMPT_CLEAR:
            // Replay overlay text is static window text, not dialog flow text.
            str[dst++] = CHAR_SPACE;
            break;
        case EXT_CTRL_CODE_BEGIN:
            // Skip inline control sequences entirely in Quest Log overlays.
            switch (str[src])
            {
            case EXT_CTRL_CODE_COLOR_HIGHLIGHT_SHADOW:
                src += 4;
                break;
            case EXT_CTRL_CODE_PLAY_BGM:
            case EXT_CTRL_CODE_PLAY_SE:
                src += 3;
                break;
            case EXT_CTRL_CODE_COLOR:
            case EXT_CTRL_CODE_HIGHLIGHT:
            case EXT_CTRL_CODE_SHADOW:
            case EXT_CTRL_CODE_PALETTE:
            case EXT_CTRL_CODE_FONT:
            case EXT_CTRL_CODE_PAUSE:
            case EXT_CTRL_CODE_ESCAPE:
            case EXT_CTRL_CODE_SHIFT_RIGHT:
            case EXT_CTRL_CODE_SHIFT_DOWN:
            case EXT_CTRL_CODE_CLEAR:
            case EXT_CTRL_CODE_SKIP:
            case EXT_CTRL_CODE_CLEAR_TO:
            case EXT_CTRL_CODE_MIN_LETTER_SPACING:
                src += 2;
                break;
            case EXT_CTRL_CODE_RESET_FONT:
            case EXT_CTRL_CODE_PAUSE_UNTIL_PRESS:
            case EXT_CTRL_CODE_WAIT_SE:
            case EXT_CTRL_CODE_FILL_WINDOW:
            case EXT_CTRL_CODE_JPN:
            case EXT_CTRL_CODE_ENG:
            case EXT_CTRL_CODE_PAUSE_MUSIC:
            case EXT_CTRL_CODE_RESUME_MUSIC:
                src += 1;
                break;
            default:
                break;
            }
            break;
        case CHAR_NEWLINE:
            if (newlineCount < maxNewlines)
            {
                str[dst++] = c;
                newlineCount++;
            }
            else
            {
                str[dst++] = CHAR_SPACE;
            }
            break;
        default:
            str[dst++] = c;
            break;
        }
    }

    str[dst] = EOS;
}

static void CopyDescriptionWindowTiles(u8 windowId)
{
    const u16 *src = sDescriptionWindow_Gfx;
    u16 *buffer = Alloc(DESC_WIN_SIZE);
    u8 i, j, y;

    if (buffer)
    {
        for (i = 0; i < DESC_WIN_HEIGHT; i++)
        {
            switch (i)
            {
            default:
                // Middle tile
                y = 1;
                break;
            case 0:
                // Top edge tile
                y = 0;
                break;
            case DESC_WIN_HEIGHT - 1:
                // Bottom edge tile
                y = 2;
                break;
            }

            for (j = 0; j < DESC_WIN_WIDTH; j++)
                CpuCopy32(src + 16 * y, buffer + 16 * (2 * (15 * i) + j), TILE_SIZE_4BPP);
        }

        CopyToWindowPixelBuffer(windowId, (const u8 *)buffer, DESC_WIN_SIZE, 0);
        Free(buffer);
    }
}

static void QuestLog_CloseTextWindow(void)
{
    ClearWindowTilemap(sWindowIds[WIN_DESCRIPTION]);
    FillWindowPixelRect(sWindowIds[WIN_DESCRIPTION], 15, 0, 0, 0xf0, 0x30);
    CopyWindowToVram(sWindowIds[WIN_DESCRIPTION], COPYWIN_GFX);
    PutWindowTilemap(sWindowIds[WIN_BOTTOM_BAR]);
    CopyWindowToVram(sWindowIds[WIN_BOTTOM_BAR], COPYWIN_MAP);
}

static void DoSkipToEndTransition(s8 delay)
{
    FadeScreen(FADE_TO_BLACK, delay);
    sQuestLogCB = QuestLog_WaitFadeAndCancelPlayback;
}

static void QuestLog_WaitFadeAndCancelPlayback(void)
{
    if (!gPaletteFade.active)
    {
        LockPlayerFieldControls();
        for (sCurrentSceneNum = sCurrentSceneNum; sCurrentSceneNum < QUEST_LOG_SCENE_COUNT; sCurrentSceneNum++)
        {
            if (gSaveBlock1Ptr->questLog[sCurrentSceneNum].startType == 0)
                break;
            if (!ReadQuestLogScriptFromSav1(sCurrentSceneNum, sQuestLogActionRecordBuffer))
            {
                AbortQuestLogPlaybackAndContinue();
                return;
            }
        }
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        QuestLog_StartFinalScene();
    }
}

void QuestLog_InitPalettesBackup(void)
{
    if (gQuestLogState == QL_STATE_PLAYBACK_LAST && sPalettesBackup == NULL)
        sPalettesBackup = AllocZeroed(PLTT_SIZE);
}

void QuestLog_BackUpPalette(u16 offset, u16 size)
{
    if (sPalettesBackup == NULL)
        return;
    CpuCopy16(&gPlttBufferUnfaded[offset], &sPalettesBackup[offset], PLTT_SIZEOF(size));
}

static bool8 FieldCB2_FinalScene(void)
{
    LoadPalette(GetTextWindowPalette(4), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    DrawPreviouslyOnQuestHeader(0);
    FieldCB_WarpExitFadeFromBlack();
    CreateTask(Task_FinalScene_WaitFade, 0xFF);
    return TRUE;
}

static void Task_FinalScene_WaitFade(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (ArePlayerFieldControlsLocked() != TRUE)
    {
        FreezeObjectEvents();
        HandleEnforcedLookDirectionOnPlayerStopMoving();
        StopPlayerAvatar();
        LockPlayerFieldControls();
        task->func = Task_QuestLogScene_SavedGame;
    }
}

static void Task_QuestLogScene_SavedGame(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (!gPaletteFade.active)
    {
        if (sPlaybackControl.endMode != END_MODE_FINISH)
        {
            GetMapNameGeneric(gStringVar1, gMapHeader.regionMapSectionId);
            StringExpandPlaceholders(gStringVar4, gText_QuestLog_SavedGameAtLocation);
            DrawSceneDescription();
            AppendQuestLogPlaybackTrace("saved_game_post_draw");
        }
        task->data[0] = 0;
        task->data[1] = 0;
        task->func = Task_WaitAtEndOfQuestLog;
        AppendQuestLogPlaybackTrace("saved_game_task_armed");
        FreezeObjectEvents();
        LockPlayerFieldControls();
        AppendQuestLogPlaybackTrace("saved_game_locked");
    }
}

#define tTimer data[0]

static void Task_WaitAtEndOfQuestLog(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    AppendQuestLogPlaybackTrace("wait_end_tick");

    if (JOY_NEW(A_BUTTON | B_BUTTON) || task->tTimer >= 127 || sPlaybackControl.endMode == END_MODE_FINISH)
    {
        QuestLog_CloseTextWindow();
        task->tTimer = 0;
        task->func = Task_EndQuestLog;
        gQuestLogState = 0;
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        sQuestLogCB = NULL;
        AppendQuestLogPlaybackTrace("wait_end_advance");
    }
    else
        task->tTimer++;
}

#undef tTimer

#define tState data[0]
#define tTimer data[1]

static void Task_EndQuestLog(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 i;

    AppendQuestLogPlaybackTaskTrace("end_replay_tick", tState, tTimer);

    switch (tState)
    {
    case 0:
        gDisableMapMusicChangeOnMapLoad = 0;
        Overworld_PlaySpecialMapMusic();
        QL_SlightlyDarkenSomePals();
        FillWindowPixelRect(sWindowIds[WIN_TOP_BAR],
                            0xF, 0, 0,
                            sWindowTemplates[WIN_TOP_BAR].width * 8,
                            sWindowTemplates[WIN_TOP_BAR].height * 8);
        tState++;
        break;
    case 1:
        if (RestoreScreenAfterPlayback(taskId))
        {
            for (i = 0; i < WIN_COUNT; i++)
            {
                ClearWindowTilemap(sWindowIds[i]);
                CopyWindowToVram(sWindowIds[i], COPYWIN_MAP);
                RemoveWindow(sWindowIds[i]);
            }
            tTimer = 0;
            tState++;
        }
        break;
    case 2:
        if (tTimer < 32)
            tTimer++;
        else
            tState++;
        break;
    default:
        if (sPlaybackControl.endMode == END_MODE_FINISH)
            ShowMapNamePopup(TRUE);
        if (sPalettesBackup != NULL)
        {
            CpuCopy16(sPalettesBackup, gPlttBufferUnfaded, PLTT_SIZE);
            Free(sPalettesBackup);
            sPalettesBackup = NULL;
        }
        sPlaybackControl = (struct PlaybackControl){};
        ClearPlayerHeldMovementAndUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
        gTextFlags.autoScroll = FALSE;
        gGlobalFieldTintMode = QL_TINT_NONE;
        DisableWildEncounters(FALSE);
        gHelpSystemEnabled = TRUE;
        DestroyTask(taskId);
        break;
    }
}

#undef tState
#undef tTimer

#define tTimer data[1]

// Scroll the top and bottom windows offscreen and restore the screen tint to the original color.
static bool8 RestoreScreenAfterPlayback(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    AppendQuestLogPlaybackTaskTrace("restore_screen_tick", data[0], tTimer);

    if (tTimer > 15)
        return TRUE;

    CopyPaletteInvertedTint(&gPlttBufferUnfaded[BG_PLTT_ID(0) + 1], &gPlttBufferFaded[BG_PLTT_ID(0) + 1], 0xDF, 15 - tTimer);
    CopyPaletteInvertedTint(&gPlttBufferUnfaded[OBJ_PLTT_ID(0)], &gPlttBufferFaded[OBJ_PLTT_ID(0)], 0x100, 15 - tTimer);
    FillWindowPixelRect(sWindowIds[WIN_TOP_BAR],
                        0x00, 0,
                        sWindowTemplates[WIN_TOP_BAR].height * 8 - 1 - tTimer,
                        sWindowTemplates[WIN_TOP_BAR].width * 8, 1);
    FillWindowPixelRect(sWindowIds[WIN_BOTTOM_BAR],
                        0x00, 0,
                        data[1],
                        sWindowTemplates[WIN_BOTTOM_BAR].width * 8, 1);
    CopyWindowToVram(sWindowIds[WIN_TOP_BAR], COPYWIN_GFX);
    CopyWindowToVram(sWindowIds[WIN_BOTTOM_BAR], COPYWIN_GFX);
    tTimer++;
    return FALSE;
}

static void QL_SlightlyDarkenSomePals(void)
{
    u16 *buffer;

    if (sPalettesBackup == NULL)
        return;

    buffer = Alloc(PLTT_SIZE);
    if (buffer == NULL)
        return;

    CpuCopy16(sPalettesBackup, buffer, PLTT_SIZE);
    SlightlyDarkenPalsInWeather(sPalettesBackup, sPalettesBackup, 13 * 16);
    SlightlyDarkenPalsInWeather(&sPalettesBackup[OBJ_PLTT_ID(1)], &sPalettesBackup[OBJ_PLTT_ID(1)], 1 * 16);
    SlightlyDarkenPalsInWeather(&sPalettesBackup[OBJ_PLTT_ID(6)], &sPalettesBackup[OBJ_PLTT_ID(6)], 4 * 16);
    SlightlyDarkenPalsInWeather(&sPalettesBackup[OBJ_PLTT_ID(11)], &sPalettesBackup[OBJ_PLTT_ID(11)], 5 * 16);
    CpuCopy16(sPalettesBackup, gPlttBufferUnfaded, PLTT_SIZE);
    CpuCopy16(buffer, sPalettesBackup, PLTT_SIZE);
    Free(buffer);
}

void QL_FinishRecordingScene(void)
{
    if (gQuestLogState == QL_STATE_RECORDING)
    {
        TryRecordActionSequence(sQuestLogActionRecordBuffer);
        RecordSceneEnd();
        gQuestLogState = 0;
        sQuestLogCB = NULL;
        gQuestLogDefeatedWildMonRecord = NULL;
        gQuestLogRecordingPointer = NULL;
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
    }
}

void QuestLog_CutRecording(void)
{
    if (gQuestLogPlaybackState != QL_PLAYBACK_STATE_STOPPED && gQuestLogState == QL_STATE_RECORDING)
    {
        TryRecordActionSequence(sQuestLogActionRecordBuffer);
        QL_RecordWait(1);
        RecordSceneEnd();
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        gQuestLogState = 0;
        sQuestLogCB = NULL;
    }
    gQuestLogDefeatedWildMonRecord = NULL;
    gQuestLogRecordingPointer = NULL;
}

static void SortQuestLogInSav1(void)
{
    struct QuestLogScene * buffer = AllocZeroed(sizeof(gSaveBlock1Ptr->questLog));
    u8 i;
    u8 sceneNum = sCurrentSceneNum;
    u8 count = 0;
    for (i = 0; i < QUEST_LOG_SCENE_COUNT; i++)
    {
        if (sceneNum >= QUEST_LOG_SCENE_COUNT)
            sceneNum = 0;
        if (gSaveBlock1Ptr->questLog[sceneNum].startType != 0)
        {
            buffer[count] = gSaveBlock1Ptr->questLog[sceneNum];
            count++;
        }
        sceneNum++;
    }
    sCurrentSceneNum = count % QUEST_LOG_SCENE_COUNT;
    CpuCopy16(buffer, gSaveBlock1Ptr->questLog, sizeof(gSaveBlock1Ptr->questLog));
    Free(buffer);
}

void SaveQuestLogData(void)
{
    if (MenuHelpers_IsLinkActive() != TRUE)
    {
        QuestLog_CutRecording();
        SortQuestLogInSav1();
    }
}

void QL_UpdateObject(struct Sprite *sprite)
{
    struct ObjectEvent *objectEvent = &gObjectEvents[sprite->data[0]];
    if (objectEvent->localId == LOCALID_PLAYER)
    {
        if (sMovementScripts[0][0] != MOVEMENT_ACTION_NONE)
        {
            ObjectEventSetHeldMovement(objectEvent, sMovementScripts[0][0]);
            sMovementScripts[0][0] = MOVEMENT_ACTION_NONE;
        }
        if (sMovementScripts[0][1] != QL_PLAYER_GFX_NONE)
        {
            QuestLogUpdatePlayerSprite(sMovementScripts[0][1]);
            sMovementScripts[0][1] = QL_PLAYER_GFX_NONE;
        }
        QL_UpdateObjectEventCurrentMovement(objectEvent, sprite);
    }
    else
    {
        if (sMovementScripts[objectEvent->localId][0] != MOVEMENT_ACTION_NONE)
        {
            ObjectEventSetHeldMovement(objectEvent, sMovementScripts[objectEvent->localId][0]);
            sMovementScripts[objectEvent->localId][0] = MOVEMENT_ACTION_NONE;
        }
        QL_UpdateObjectEventCurrentMovement(objectEvent, sprite);
    }
}

void QuestLogRecordNPCStep(u8 localId, u8 mapNum, u8 mapGroup, u8 movementActionId)
{
    if (!RecordHeadAtEndOfEntryOrScriptContext2Enabled())
    {
        sCurSceneActions[gQuestLogCurActionIdx].duration = sNextActionDelay;
        sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_MOVEMENT;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.localId = localId;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.mapNum = mapNum;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.mapGroup = mapGroup;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId = movementActionId;
        gQuestLogCurActionIdx++;
        sNextActionDelay = 0;
    }
}

void QuestLogRecordNPCStepWithDuration(u8 localId, u8 mapNum, u8 mapGroup, u8 movementActionId, u8 duration)
{
    if (!RecordHeadAtEndOfEntry())
    {
        sCurSceneActions[gQuestLogCurActionIdx].duration = sNextActionDelay;
        sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_MOVEMENT;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.localId = localId;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.mapNum = mapNum;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.mapGroup = mapGroup;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId = movementActionId;
        gQuestLogCurActionIdx++;
        sNextActionDelay = duration;
    }
}

void QuestLogRecordPlayerStep(u8 movementActionId)
{
    if (!RecordHeadAtEndOfEntryOrScriptContext2Enabled())
    {
        if (movementActionId != sCurSceneActions[sLastQuestLogCursor].data.a.movementActionId || movementActionId > MOVEMENT_ACTION_FACE_RIGHT)
        {
            sCurSceneActions[gQuestLogCurActionIdx].duration = sNextActionDelay;
            sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_MOVEMENT;
            sCurSceneActions[gQuestLogCurActionIdx].data.a.localId = 0;
            sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId = movementActionId;
            sLastQuestLogCursor = gQuestLogCurActionIdx;
            gQuestLogCurActionIdx++;
            sNextActionDelay = 0;
        }
    }
}

void QuestLogRecordPlayerStepWithDuration(u8 movementActionId, u8 duration)
{
    if (!RecordHeadAtEndOfEntry())
    {
        sCurSceneActions[gQuestLogCurActionIdx].duration = sNextActionDelay;
        sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_MOVEMENT;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.localId = 0;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId = movementActionId;
        sLastQuestLogCursor = gQuestLogCurActionIdx;
        gQuestLogCurActionIdx++;
        sNextActionDelay = duration;
    }
}

void QuestLogRecordPlayerAvatarGfxTransition(u8 gfxState)
{
    if (!RecordHeadAtEndOfEntry())
    {
        sCurSceneActions[gQuestLogCurActionIdx].duration = sNextActionDelay;
        sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_GFX_CHANGE;
        sCurSceneActions[gQuestLogCurActionIdx].data.b.localId = 0;
        sCurSceneActions[gQuestLogCurActionIdx].data.b.gfxState = gfxState;
        gQuestLogCurActionIdx++;
        sNextActionDelay = 0;
    }
}

void QuestLogRecordPlayerAvatarGfxTransitionWithDuration(u8 gfxState, u8 duration)
{
    if (!RecordHeadAtEndOfEntry())
    {
        sCurSceneActions[gQuestLogCurActionIdx].duration = sNextActionDelay;
        sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_GFX_CHANGE;
        sCurSceneActions[gQuestLogCurActionIdx].data.b.localId = 0;
        sCurSceneActions[gQuestLogCurActionIdx].data.b.gfxState = gfxState;
        gQuestLogCurActionIdx++;
        sNextActionDelay = duration;
    }
}

void QL_RecordFieldInput(struct FieldInput * fieldInput)
{
    if (gQuestLogCurActionIdx < sMaxActionsInScene)
    {
        // Retain only the following fields:
        // - pressedAButton
        // - checkStandardWildEncounter
        // - heldDirection
        // - heldDirection2
        // - tookStep
        // - pressedBButton
        // - dpadDirection
        u32 data = *(u32 *)fieldInput & 0x00FF00F3;
        sCurSceneActions[gQuestLogCurActionIdx].duration = sNextActionDelay;
        sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_INPUT;
        sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[0] = data;
        sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[1] = data >> 8; // always 0
        sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[2] = data >> 16;
        sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[3] = data >> 24; // always 0
        gQuestLogCurActionIdx++;
        if (ArePlayerFieldControlsLocked())
            sNextActionDelay = 1;
        else
            sNextActionDelay = 0;
    }
}

static void TogglePlaybackStateForOverworldLock(u8 a0)
{
    switch (a0)
    {
    case 1:
        if (gQuestLogPlaybackState == QL_PLAYBACK_STATE_RUNNING)
            gQuestLogPlaybackState = QL_PLAYBACK_STATE_ACTION_END; // Message visible, overworld locked
        break;
    case 2:
        if (gQuestLogPlaybackState == QL_PLAYBACK_STATE_ACTION_END)
            gQuestLogPlaybackState = QL_PLAYBACK_STATE_RUNNING; // Overworld unlocked
        break;
    }
}

void QuestLog_OnEscalatorWarp(u8 direction)
{
    u8 state = QL_GetPlaybackState();

    switch (direction)
    {
    case QL_ESCALATOR_OUT: // warp out
        if (state == QL_PLAYBACK_STATE_RUNNING)
            gQuestLogPlaybackState = QL_PLAYBACK_STATE_ACTION_END;
        else if (state == QL_PLAYBACK_STATE_RECORDING)
        {
            sCurSceneActions[gQuestLogCurActionIdx].duration = sNextActionDelay;
            sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_EMPTY;
            gQuestLogCurActionIdx++;
            sNextActionDelay = 0;
            gQuestLogPlaybackState = QL_PLAYBACK_STATE_RECORDING_NO_DELAY;
        }
        break;
    case QL_ESCALATOR_IN: // warp in
        if (state == QL_PLAYBACK_STATE_RUNNING)
            gQuestLogPlaybackState = QL_PLAYBACK_STATE_RUNNING;
        else if (state == QL_PLAYBACK_STATE_RECORDING)
            gQuestLogPlaybackState = QL_PLAYBACK_STATE_RECORDING;
        break;
    }
}

static void ResetActions(u8 kind, struct QuestLogAction *actions, u16 size)
{
    int i;

    switch (kind)
    {
    default:
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        break;
    case QL_PLAYBACK_STATE_RUNNING:
        sCurSceneActions = actions;
        sMaxActionsInScene = size / sizeof(*sCurSceneActions);
        for (i = 0; i < (s32)ARRAY_COUNT(sMovementScripts); i++)
        {
            sMovementScripts[i][0] |= MOVEMENT_ACTION_NONE;
            sMovementScripts[i][1] |= QL_PLAYER_GFX_NONE;
        }
        gQuestLogCurActionIdx = 0;
        sLastQuestLogCursor = 0;
        gQuestLogFieldInput = (struct FieldInput){};
        sNextActionDelay = sCurSceneActions[gQuestLogCurActionIdx].duration;
        sMovementScripts[0][0] = sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId;
        sMovementScripts[0][1] = QL_PLAYER_GFX_NONE;
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_RUNNING;
        break;
    case QL_PLAYBACK_STATE_RECORDING:
        sCurSceneActions = actions;
        sMaxActionsInScene = size / sizeof(*sCurSceneActions);
        for (i = 0; i < sMaxActionsInScene; i++)
        {
            sCurSceneActions[i] = (struct QuestLogAction){
                .duration = 0xFFFF,
                .type = QL_ACTION_SCENE_END
            };
        }
        gQuestLogCurActionIdx = 0;
        sNextActionDelay = 0;
        sCurSceneActions[gQuestLogCurActionIdx].duration = 0;
        sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_MOVEMENT;
        sCurSceneActions[gQuestLogCurActionIdx].data.a.localId = 0;
        switch (GetPlayerFacingDirection())
        {
        case DIR_NONE:
        case DIR_SOUTH:
            sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId = MOVEMENT_ACTION_FACE_DOWN;
            break;
        case DIR_EAST:
            sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId = MOVEMENT_ACTION_FACE_RIGHT;
            break;
        case DIR_NORTH:
            sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId = MOVEMENT_ACTION_FACE_UP;
            break;
        case DIR_WEST:
            sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId = MOVEMENT_ACTION_FACE_LEFT;
            break;
        }
        sLastQuestLogCursor = 0;
        gQuestLogCurActionIdx++;
        sCurSceneActions[gQuestLogCurActionIdx].duration = 0;
        sCurSceneActions[gQuestLogCurActionIdx].type = QL_ACTION_INPUT;
        sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[0] = 0;
        sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[1] = 0;
        sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[2] = 0;
        sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[3] = 0;
        gQuestLogCurActionIdx++;
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_RECORDING;
        break;
    }
}

void QL_TryRunActions(void)
{
    switch (gQuestLogPlaybackState)
    {
    case QL_PLAYBACK_STATE_STOPPED:
    case QL_PLAYBACK_STATE_ACTION_END:
    case QL_PLAYBACK_STATE_RECORDING_NO_DELAY:
        break;
    case QL_PLAYBACK_STATE_RUNNING:
        if (!RecordHeadAtEndOfEntryOrScriptContext2Enabled())
        {
            if (sNextActionDelay != 0)
            {
                sNextActionDelay--;
            }
            else
            {
                do
                {
                    AppendQuestLogPlaybackActionTrace("ql_tryrun_execute_action", &sCurSceneActions[gQuestLogCurActionIdx]);
                    switch (sCurSceneActions[gQuestLogCurActionIdx].type)
                    {
                    case QL_ACTION_MOVEMENT:
                        // NPC movement action
                        sMovementScripts[sCurSceneActions[gQuestLogCurActionIdx].data.a.localId][0] = sCurSceneActions[gQuestLogCurActionIdx].data.a.movementActionId;
                        break;
                    case QL_ACTION_GFX_CHANGE:
                        // State transition
                        sMovementScripts[sCurSceneActions[gQuestLogCurActionIdx].data.b.localId][1] = sCurSceneActions[gQuestLogCurActionIdx].data.b.gfxState;
                        break;
                    case QL_ACTION_INPUT:
                        // Player input
                        *(u32 *)&gQuestLogFieldInput = ((sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[3] << 24)
                                                      | (sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[2] << 16)
                                                      | (sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[1] << 8)
                                                      | (sCurSceneActions[gQuestLogCurActionIdx].data.fieldInput[0] << 0));
                        break;
                    case QL_ACTION_EMPTY:
                        // End
                        gQuestLogPlaybackState = QL_PLAYBACK_STATE_ACTION_END;
                        break;
                    case QL_ACTION_WAIT:
                        // Nothing. The wait action uses sNextActionDelay to add a pause to playback.
                        // When the counter is finished and this is reached there's nothing else that needs to be done.
                        break;
                    case QL_ACTION_SCENE_END:
                        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
                        break;
                    }

                    if (gQuestLogPlaybackState == QL_PLAYBACK_STATE_STOPPED)
                        break;
                    if (++gQuestLogCurActionIdx >= sMaxActionsInScene)
                    {
                        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
                        break;
                    }
                    sNextActionDelay = sCurSceneActions[gQuestLogCurActionIdx].duration;
                    AppendQuestLogPlaybackActionTrace("ql_tryrun_next_action", &sCurSceneActions[gQuestLogCurActionIdx]);

                } while (gQuestLogPlaybackState != QL_PLAYBACK_STATE_ACTION_END && (sNextActionDelay == 0 || sNextActionDelay == 0xFFFF));
            }
        }
        else if (gQuestLogCurActionIdx >= sMaxActionsInScene)
        {
            gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        }
        break;
    case QL_PLAYBACK_STATE_RECORDING:
        if (ArePlayerFieldControlsLocked() != TRUE)
        {
            sNextActionDelay++;
            if (gQuestLogCurActionIdx >= sMaxActionsInScene)
                gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
        }
        break;
    }
}

void QL_AfterRecordFishActionSuccessful(void)
{
    sNextActionDelay++;
}

u8 QL_GetPlaybackState(void)
{
    switch (gQuestLogPlaybackState)
    {
    case QL_PLAYBACK_STATE_STOPPED:
    default:
        return QL_PLAYBACK_STATE_STOPPED;

    case QL_PLAYBACK_STATE_RUNNING:
    case QL_PLAYBACK_STATE_ACTION_END:
        return QL_PLAYBACK_STATE_RUNNING;

    case QL_PLAYBACK_STATE_RECORDING:
    case QL_PLAYBACK_STATE_RECORDING_NO_DELAY:
        return QL_PLAYBACK_STATE_RECORDING;
    }
}

static bool8 RecordHeadAtEndOfEntryOrScriptContext2Enabled(void)
{
    if (gQuestLogCurActionIdx >= sMaxActionsInScene)
        return TRUE;
    if (gQuestLogState != QL_STATE_PLAYBACK && ArePlayerFieldControlsLocked() == TRUE)
        return TRUE;
    return FALSE;
}

static bool8 RecordHeadAtEndOfEntry(void)
{
    if (gQuestLogCurActionIdx >= sMaxActionsInScene)
        return TRUE;
    return FALSE;
}

static const struct FlagOrVarRecord sDummyFlagOrVarRecord = {
    .idx = 0,
    .isFlag = FALSE,
    .value = 0x7FFF
};

void *QuestLogGetFlagOrVarPtr(bool8 isFlag, u16 idx)
{
    void *response;
    if (gQuestLogCurActionIdx == 0)
        return NULL;
    if (gQuestLogCurActionIdx >= sMaxActionsInScene)
        return NULL;
    if (sFlagOrVarPlayhead >= sNumFlagsOrVars)
        return NULL;
    if (sFlagOrVarRecords[sFlagOrVarPlayhead].idx == idx && sFlagOrVarRecords[sFlagOrVarPlayhead].isFlag == isFlag)
    {
        response = &sFlagOrVarRecords[sFlagOrVarPlayhead].value;
        sFlagOrVarPlayhead++;
    }
    else
        response = NULL;
    return response;
}

void QuestLogSetFlagOrVar(bool8 isFlag, u16 idx, u16 value)
{
    if (gQuestLogCurActionIdx == 0)
        return;
    if (gQuestLogCurActionIdx >= sMaxActionsInScene)
        return;
    if (sFlagOrVarPlayhead >= sNumFlagsOrVars)
        return;
    sFlagOrVarRecords[sFlagOrVarPlayhead].idx = idx;
    sFlagOrVarRecords[sFlagOrVarPlayhead].isFlag = isFlag;
    sFlagOrVarRecords[sFlagOrVarPlayhead].value = value;
    sFlagOrVarPlayhead++;
}

// Unused
static void QuestLogResetFlagsOrVars(u8 state, struct FlagOrVarRecord * records, u16 size)
{
    s32 i;

    if (state == 0 || state > QL_STATE_PLAYBACK)
    {
        gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
    }
    else
    {
        sFlagOrVarRecords = records;
        sNumFlagsOrVars = size / 4;
        sFlagOrVarPlayhead = 0;
        if (state == QL_STATE_PLAYBACK)
        {
            for (i = 0; i < sMaxActionsInScene; i++)
                sFlagOrVarRecords[i] = sDummyFlagOrVarRecord;
        }
    }
}
