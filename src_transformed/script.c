#include "global.h"
#include "script.h"
#include "event_data.h"
#include "quest_log.h"
#include "mystery_gift.h"
#include "constants/maps.h"
#include "constants/map_scripts.h"

#ifdef PORTABLE
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

extern void firered_runtime_trace_external(const char *message);

#ifndef NDEBUG
extern char *getenv(const char *name);

static int TraceScriptEnvEnabled(void)
{
    static int s_init;
    static int s_on;
    const char *e;

    if (s_init)
        return s_on;
    s_init = 1;
    e = getenv("FIRERED_TRACE_SCRIPT");
    s_on = (e != NULL && e[0] != '\0' && e[0] != '0');
    return s_on;
}
#else
static int TraceScriptEnvEnabled(void)
{
    return 0;
}
#endif
extern const u8 PalletTown_EventScript_OakTrigger[];
extern const u8 PalletTown_EventScript_OakTriggerLeft[];
extern const u8 PalletTown_EventScript_OakTriggerRight[];
extern const u8 PalletTown_ProfessorOaksLab_EventScript_BulbasaurBall[];
extern const u8 PalletTown_ProfessorOaksLab_EventScript_CharmanderBall[];
extern const u8 PalletTown_ProfessorOaksLab_EventScript_SquirtleBall[];
extern const u8 PalletTown_ProfessorOaksLab_EventScript_ConfirmBulbasaur[];
extern const u8 PalletTown_ProfessorOaksLab_EventScript_ConfirmCharmander[];
extern const u8 PalletTown_ProfessorOaksLab_EventScript_ConfirmSquirtle[];
extern const u8 PalletTown_ProfessorOaksLab_EventScript_ChoseStarter[];
extern const u8 PalletTown_ProfessorOaksLab_EventScript_RivalPicksStarter[];
extern const u8 PalletTown_ProfessorOaksLab_EventScript_RivalTakesStarter[];

static bool8 sTraceOakScript;
static u8 sOakTraceCount;

static void TraceOakScript(const char *fmt, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    firered_runtime_trace_external(buffer);
}

static bool8 ShouldTraceOakScript(const u8 *script)
{
    return script == PalletTown_EventScript_OakTrigger
        || script == PalletTown_EventScript_OakTriggerLeft
        || script == PalletTown_EventScript_OakTriggerRight
        || script == PalletTown_ProfessorOaksLab_EventScript_BulbasaurBall
        || script == PalletTown_ProfessorOaksLab_EventScript_CharmanderBall
        || script == PalletTown_ProfessorOaksLab_EventScript_SquirtleBall
        || script == PalletTown_ProfessorOaksLab_EventScript_ConfirmBulbasaur
        || script == PalletTown_ProfessorOaksLab_EventScript_ConfirmCharmander
        || script == PalletTown_ProfessorOaksLab_EventScript_ConfirmSquirtle
        || script == PalletTown_ProfessorOaksLab_EventScript_ChoseStarter
        || script == PalletTown_ProfessorOaksLab_EventScript_RivalPicksStarter
        || script == PalletTown_ProfessorOaksLab_EventScript_RivalTakesStarter;
}

static const u8 *ResolveMapScriptPointer(const u8 *script)
{
    uintptr_t value;
    uint32_t rawValue;

    if (script == NULL)
        return NULL;

    value = (uintptr_t)script;
    if (value <= UINT32_MAX)
    {
        rawValue = (uint32_t)value;
        return (const u8 *)firered_portable_resolve_script_ptr(rawValue);
    }

    return script;
}
#endif

extern void ResetContextNpcTextColor(void); // field_specials
extern u16 CalcCRC16WithTable(u8 *data, int length); // util

#define RAM_SCRIPT_MAGIC 51

enum {
    SCRIPT_MODE_STOPPED,
    SCRIPT_MODE_BYTECODE,
    SCRIPT_MODE_NATIVE,
};

enum {
    CONTEXT_RUNNING,
    CONTEXT_WAITING,
    CONTEXT_SHUTDOWN,
};

EWRAM_DATA u8 gWalkAwayFromSignInhibitTimer = 0;
EWRAM_DATA const u8 *gRamScriptRetAddr = NULL;

static u8 sGlobalScriptContextStatus;
static u32 sUnusedVariable1;
static struct ScriptContext sGlobalScriptContext;
static u32 sUnusedVariable2;
static struct ScriptContext sImmediateScriptContext;
static bool8 sLockFieldControls;
static u8 sMsgBoxWalkawayDisabled;
static u8 sMsgBoxIsCancelable;
static u8 sQuestLogInput;
static u8 sQuestLogInputIsDpad;
static u8 sMsgIsSignpost;

#ifdef PORTABLE
extern const u8 gScriptCmdTable[];
extern const u8 gScriptCmdTableEnd[];
#else
extern ScrCmdFunc gScriptCmdTable[];
extern ScrCmdFunc gScriptCmdTableEnd[];
#endif
extern void *gNullScriptPtr;

void InitScriptContext(struct ScriptContext *ctx, const void *cmdTable, const void *cmdTableEnd)
{
    s32 i;

    ctx->mode = SCRIPT_MODE_STOPPED;
    ctx->scriptPtr = NULL;
    ctx->stackDepth = 0;
    ctx->nativePtr = NULL;
    ctx->cmdTable = cmdTable;
    ctx->cmdTableEnd = cmdTableEnd;

    for (i = 0; i < (int)ARRAY_COUNT(ctx->data); i++)
        ctx->data[i] = 0;

    for (i = 0; i < (int)ARRAY_COUNT(ctx->stack); i++)
        ctx->stack[i] = 0;
}

u8 SetupBytecodeScript(struct ScriptContext *ctx, const u8 *ptr)
{
    ctx->scriptPtr = ptr;
    ctx->mode = SCRIPT_MODE_BYTECODE;
    return 1;
}

void SetupNativeScript(struct ScriptContext *ctx, bool8 (*ptr)(void))
{
    ctx->mode = SCRIPT_MODE_NATIVE;
    ctx->nativePtr = ptr;
}

void StopScript(struct ScriptContext *ctx)
{
    ctx->mode = SCRIPT_MODE_STOPPED;
    ctx->scriptPtr = NULL;
}

bool8 RunScriptCommand(struct ScriptContext *ctx)
{
    // FRLG disabled this check, where-as it is present
    // in Ruby/Sapphire and Emerald. Why did the programmers
    // bother to remove a redundant check when it still
    // exists in Emerald?
    //if (ctx->mode == SCRIPT_MODE_STOPPED)
    //    return FALSE;

    switch (ctx->mode)
    {
    case SCRIPT_MODE_STOPPED:
        return FALSE;
    case SCRIPT_MODE_NATIVE:
        // Try to call a function in C
        // Continue to bytecode if no function or it returns TRUE
        if (ctx->nativePtr)
        {
            bool8 nativeResult = ctx->nativePtr();
            if (nativeResult == TRUE)
                ctx->mode = SCRIPT_MODE_BYTECODE;
            return TRUE;
        }
        ctx->mode = SCRIPT_MODE_BYTECODE;
        // fallthrough
    case SCRIPT_MODE_BYTECODE:
        while (1)
        {
            u8 cmdCode;
#ifdef PORTABLE
            ScrCmdFunc cmdFunc;
            u32 rawCmdToken;
#else
            ScrCmdFunc *cmdFunc;
#endif
#ifdef PORTABLE
            const u8 *cmdTableBytes;
            const u8 *cmdTableEndBytes;
#endif

            if (ctx->scriptPtr == NULL)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }

            if (ctx->scriptPtr == gNullScriptPtr)
            {
#ifdef PORTABLE
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
#else
                while (1)
                    asm("svc 2"); // HALT
#endif
            }

            cmdCode = *(ctx->scriptPtr);
#ifdef PORTABLE
    if (sTraceOakScript && sOakTraceCount < 192)
            {
                TraceOakScript("OakScript: cmd=%02X ptr=%p", cmdCode, ctx->scriptPtr);
                sOakTraceCount++;
            }
#endif
            ctx->scriptPtr++;

#ifdef PORTABLE
            cmdTableBytes = (const u8 *)ctx->cmdTable;
            cmdTableEndBytes = (const u8 *)ctx->cmdTableEnd;
            if (cmdTableBytes + cmdCode * 4 >= cmdTableEndBytes)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }
            rawCmdToken = T2_READ_32(cmdTableBytes + cmdCode * 4);
            cmdFunc = (ScrCmdFunc)firered_portable_resolve_script_ptr(rawCmdToken);
#else
            cmdFunc = &ctx->cmdTable[cmdCode];
#endif

#ifndef PORTABLE
            if (cmdFunc >= ctx->cmdTableEnd)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }
            if ((*cmdFunc)(ctx) == TRUE)
            {
                return TRUE;
            }
#else
            if (cmdFunc == NULL)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }
            if (cmdFunc(ctx) == TRUE)
            {
                return TRUE;
            }
#endif
        }
    }

    return TRUE;
}

static u8 ScriptPush(struct ScriptContext *ctx, const u8 *ptr)
{
    if (ctx->stackDepth + 1 >= (int)ARRAY_COUNT(ctx->stack))
    {
        return 1;
    }
    else
    {
        ctx->stack[ctx->stackDepth] = ptr;
        ctx->stackDepth++;
        return 0;
    }
}

static const u8 *ScriptPop(struct ScriptContext *ctx)
{
    if (ctx->stackDepth == 0)
        return NULL;

    ctx->stackDepth--;
    return ctx->stack[ctx->stackDepth];
}

void ScriptJump(struct ScriptContext *ctx, const u8 *ptr)
{
    ctx->scriptPtr = ptr;
}

void ScriptCall(struct ScriptContext *ctx, const u8 *ptr)
{
    ScriptPush(ctx, ctx->scriptPtr);
    ctx->scriptPtr = ptr;
}

void ScriptReturn(struct ScriptContext *ctx)
{
    ctx->scriptPtr = ScriptPop(ctx);
}

u16 ScriptReadHalfword(struct ScriptContext *ctx)
{
    u16 value = *(ctx->scriptPtr++);
    value |= *(ctx->scriptPtr++) << 8;
    return value;
}

u32 ScriptReadWord(struct ScriptContext *ctx)
{
    u32 value0 = *(ctx->scriptPtr++);
    u32 value1 = *(ctx->scriptPtr++);
    u32 value2 = *(ctx->scriptPtr++);
    u32 value3 = *(ctx->scriptPtr++);
    return (((((value3 << 8) + value2) << 8) + value1) << 8) + value0;
}

const void *ScriptReadPtr(struct ScriptContext *ctx)
{
    const void *ptr = T2_READ_PTR(ctx->scriptPtr);
    ctx->scriptPtr += 4;
    return ptr;
}

void LockPlayerFieldControls(void)
{
    sLockFieldControls = TRUE;
}

void UnlockPlayerFieldControls(void)
{
    sLockFieldControls = FALSE;
}

bool8 ArePlayerFieldControlsLocked(void)
{
    return sLockFieldControls;
}

void SetQuestLogInputIsDpadFlag(void)
{
    sQuestLogInputIsDpad = TRUE;
}

void ClearQuestLogInputIsDpadFlag(void)
{
    sQuestLogInputIsDpad = FALSE;
}

bool8 IsQuestLogInputDpad(void)
{
    if(sQuestLogInputIsDpad == TRUE)
        return TRUE;
    else
        return FALSE;
}

void RegisterQuestLogInput(u8 var)
{
    sQuestLogInput = var;
}

void ClearQuestLogInput(void)
{
    sQuestLogInput = 0;
}

u8 GetRegisteredQuestLogInput(void)
{
    return sQuestLogInput;
}

void DisableMsgBoxWalkaway(void)
{
    sMsgBoxWalkawayDisabled = TRUE;
}

void EnableMsgBoxWalkaway(void)
{
    sMsgBoxWalkawayDisabled = FALSE;
}

bool8 IsMsgBoxWalkawayDisabled(void)
{
    return sMsgBoxWalkawayDisabled;
}

void SetWalkingIntoSignVars(void)
{
    gWalkAwayFromSignInhibitTimer = 6;
    sMsgBoxIsCancelable = TRUE;
}

void ClearMsgBoxCancelableState(void)
{
    sMsgBoxIsCancelable = FALSE;
}

bool8 CanWalkAwayToCancelMsgBox(void)
{
    if(sMsgBoxIsCancelable == TRUE)
        return TRUE;
    else
        return FALSE;
}

void MsgSetSignpost(void)
{
    sMsgIsSignpost = TRUE;
}

void MsgSetNotSignpost(void)
{
    sMsgIsSignpost = FALSE;
}

bool8 IsMsgSignpost(void)
{
    if(sMsgIsSignpost == TRUE)
        return TRUE;
    else
        return FALSE;
}

void ResetFacingNpcOrSignpostVars(void)
{
    ResetContextNpcTextColor();
    MsgSetNotSignpost();
}

// The ScriptContext_* functions work with the primary script context,
// which yields control back to native code should the script make a wait call.

// Checks if the global script context is able to be run right now.
bool8 ScriptContext_IsEnabled(void)
{
    if (sGlobalScriptContextStatus == CONTEXT_RUNNING)
        return TRUE;
    else
        return FALSE;
}

// Re-initializes the global script context to zero.
void ScriptContext_Init(void)
{
    InitScriptContext(&sGlobalScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    sGlobalScriptContextStatus = CONTEXT_SHUTDOWN;
}

// Runs the script until the script makes a wait* call, then returns true if 
// there's more script to run, or false if the script has hit the end. 
// This function also returns false if the context is finished 
// or waiting (after a call to _Stop)
bool8 ScriptContext_RunScript(void)
{
#ifdef PORTABLE
    if (TraceScriptEnvEnabled())
        firered_runtime_trace_external("CrashTrace: ScriptContext_RunScript enter");
#endif
    if (sGlobalScriptContextStatus == CONTEXT_SHUTDOWN)
        return FALSE;

    if (sGlobalScriptContextStatus == CONTEXT_WAITING)
        return FALSE;

    LockPlayerFieldControls();

    if (!RunScriptCommand(&sGlobalScriptContext))
    {
        sGlobalScriptContextStatus = CONTEXT_SHUTDOWN;
        UnlockPlayerFieldControls();
        return FALSE;
    }

    return TRUE;
}

// Sets up a new script in the global context and enables the context
void ScriptContext_SetupScript(const u8 *ptr)
{
    ClearMsgBoxCancelableState();
    EnableMsgBoxWalkaway();
    ClearQuestLogInputIsDpadFlag();

    InitScriptContext(&sGlobalScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    SetupBytecodeScript(&sGlobalScriptContext, ptr);
#ifdef PORTABLE
    sTraceOakScript = ShouldTraceOakScript(ptr);
    sOakTraceCount = 0;
    if (sTraceOakScript)
        TraceOakScript("OakScript: setup ptr=%p", ptr);
#endif
    LockPlayerFieldControls();
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
}

// Puts the script into waiting mode; usually called from a wait* script command.
void ScriptContext_Stop(void)
{
    sGlobalScriptContextStatus = CONTEXT_WAITING;
}

// Puts the script into running mode.
void ScriptContext_Enable(void)
{
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
    LockPlayerFieldControls();
}

// Sets up and runs a script in its own context immediately. The script will be
// finished when this function returns. Used mainly by all of the map header
// scripts (except the frame table scripts).
void RunScriptImmediately(const u8 *ptr)
{
    InitScriptContext(&sImmediateScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    SetupBytecodeScript(&sImmediateScriptContext, ptr);
    while (1)
    {
        if (RunScriptCommand(&sImmediateScriptContext) != TRUE)
            break;
    }
}

static u8 *MapHeaderGetScriptTable(u8 tag)
{
    const u8 *mapScripts = gMapHeader.mapScripts;

    if (mapScripts == NULL)
        return NULL;

    while (1)
    {
        if (*mapScripts == 0)
            return NULL;
        if (*mapScripts == tag)
        {
            mapScripts++;
            return T2_READ_PTR(mapScripts);
        }
        mapScripts += 5;
    }
}

static void MapHeaderRunScriptType(u8 tag)
{
    u8 *ptr = MapHeaderGetScriptTable(tag);
    if (ptr != NULL)
#ifdef PORTABLE
        RunScriptImmediately((u8 *)ResolveMapScriptPointer(ptr));
#else
        RunScriptImmediately(ptr);
#endif
}

static u8 *MapHeaderCheckScriptTable(u8 tag)
{
    u8 *ptr = MapHeaderGetScriptTable(tag);

    if (ptr == NULL)
        return NULL;

    while (1)
    {
        u16 varIndex1;
        u16 varIndex2;

        // Read first var (or .2byte terminal value)
        varIndex1 = T1_READ_16(ptr);
        if (!varIndex1)
            return NULL; // Reached end of table
        ptr += 2;

        // Read second var
        varIndex2 = T1_READ_16(ptr);
        ptr += 2;

        // Run map script if vars are equal
        if (VarGet(varIndex1) == VarGet(varIndex2))
#ifdef PORTABLE
            return (u8 *)ResolveMapScriptPointer(T2_READ_PTR(ptr));
#else
            return T2_READ_PTR(ptr);
#endif
        ptr += 4;
    }
}

void RunOnLoadMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_LOAD);
}

void RunOnTransitionMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_TRANSITION);
}

void RunOnResumeMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_RESUME);
}

void RunOnReturnToFieldMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_RETURN_TO_FIELD);
}

void RunOnDiveWarpMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_DIVE_WARP);
}

bool8 TryRunOnFrameMapScript(void)
{
    u8 *ptr;

    if (gQuestLogState == QL_STATE_PLAYBACK_LAST)
        return FALSE;

    ptr = MapHeaderCheckScriptTable(MAP_SCRIPT_ON_FRAME_TABLE);

    if (!ptr)
        return FALSE;

    ScriptContext_SetupScript(ptr);
    return TRUE;
}

void TryRunOnWarpIntoMapScript(void)
{
    u8 *ptr = MapHeaderCheckScriptTable(MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE);
    if (ptr)
        RunScriptImmediately(ptr);
}

u32 CalculateRamScriptChecksum(void)
{
    return CalcCRC16WithTable((u8 *)(&gSaveBlock1Ptr->ramScript.data), sizeof(gSaveBlock1Ptr->ramScript.data));
}

void ClearRamScript(void)
{
    CpuFill32(0, &gSaveBlock1Ptr->ramScript, sizeof(struct RamScript));
}

bool8 InitRamScript(u8 *script, u16 scriptSize, u8 mapGroup, u8 mapNum, u8 objectId)
{
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;

    ClearRamScript();

    if (scriptSize > sizeof(scriptData->script))
        return FALSE;

    scriptData->magic = RAM_SCRIPT_MAGIC;
    scriptData->mapGroup = mapGroup;
    scriptData->mapNum = mapNum;
    scriptData->objectId = objectId;
    memcpy(scriptData->script, script, scriptSize);
    gSaveBlock1Ptr->ramScript.checksum = CalculateRamScriptChecksum();
    return TRUE;
}

const u8 *GetRamScript(u8 objectId, const u8 *script)
{
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    gRamScriptRetAddr = NULL;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return script;
    if (scriptData->mapGroup != gSaveBlock1Ptr->location.mapGroup)
        return script;
    if (scriptData->mapNum != gSaveBlock1Ptr->location.mapNum)
        return script;
    if (scriptData->objectId != objectId)
        return script;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
    {
        ClearRamScript();
        return script;
    }
    else
    {
        gRamScriptRetAddr = script;
        return scriptData->script;
    }
}

bool32 ValidateRamScript(void)
{
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return FALSE;
    if (scriptData->mapGroup != MAP_GROUP(MAP_UNDEFINED))
        return FALSE;
    if (scriptData->mapNum != MAP_NUM(MAP_UNDEFINED))
        return FALSE;
    if (scriptData->objectId != 0xFF)
        return FALSE;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
        return FALSE;
    return TRUE;
}

u8 *GetSavedRamScriptIfValid(void)
{
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    if (!ValidateSavedWonderCard())
        return NULL;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return NULL;
    if (scriptData->mapGroup != MAP_GROUP(MAP_UNDEFINED))
        return NULL;
    if (scriptData->mapNum != MAP_NUM(MAP_UNDEFINED))
        return NULL;
    if (scriptData->objectId != 0xFF)
        return NULL;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
    {
        ClearRamScript();
        return NULL;
    }
    else
    {
        return scriptData->script;
    }
}

void InitRamScript_NoObjectEvent(u8 *script, u16 scriptSize)
{
    if (scriptSize > sizeof(gSaveBlock1Ptr->ramScript.data.script))
        scriptSize = sizeof(gSaveBlock1Ptr->ramScript.data.script);
    InitRamScript(script, scriptSize, MAP_GROUP(MAP_UNDEFINED), MAP_NUM(MAP_UNDEFINED), 0xFF);
}
