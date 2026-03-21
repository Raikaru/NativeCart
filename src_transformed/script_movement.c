#include "global.h"
#include "task.h"
#include "util.h"
#include "event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"

extern void engine_backend_trace_external(const char *message);

#ifdef PORTABLE
#include <stdarg.h>
#include <stdio.h>

extern void firered_runtime_trace_external(const char *message);

#ifndef NDEBUG
extern char *getenv(const char *name);

static int TraceMoveScriptEnvEnabled(void)
{
    static int s_init;
    static int s_on;
    const char *e;

    if (s_init)
        return s_on;
    s_init = 1;
    e = getenv("FIRERED_TRACE_MOVE_SCRIPT");
    s_on = (e != NULL && e[0] != '\0' && e[0] != '0');
    return s_on;
}
#else
static int TraceMoveScriptEnvEnabled(void)
{
    return 0;
}
#endif

static void TraceMovementScript(const char *fmt, ...)
{
    char buffer[256];
    va_list args;

    if (!TraceMoveScriptEnvEnabled())
        return;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    firered_runtime_trace_external(buffer);
}
#endif

static EWRAM_DATA const u8 (*sMovementScripts[OBJECT_EVENTS_COUNT]) = {};

static void ScriptMovement_StartMoveObjects(u8 priority);
static u8 GetMoveObjectsTaskId(void);
static u8 ScriptMovement_TryAddNewMovement(u8 taskId, u8 objEventId, const u8 *movementScript);
static u8 GetMovementScriptIdFromObjectEventId(u8 taskId, u8 objEventId);
static bool8 IsMovementScriptFinished(u8 taskId, u8 moveScrId);
static void ScriptMovement_MoveObjects(u8 taskId);
static void ScriptMovement_AddNewMovement(u8 taskId, u8 moveScrId, u8 objEventId, const u8 *movementScript);
static void ScriptMovement_UnfreezeActiveObjects(u8 taskId);
static void ScriptMovement_TakeStep(u8 taskId, u8 moveScrId, u8 objEventId, const u8 *movementScript);

bool8 ScriptMovement_StartObjectMovementScript(u8 localId, u8 mapNum, u8 mapGroup, const u8 *movementScript)
{
    u8 objEventId;
    if (TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objEventId))
        return TRUE;

    if (!FuncIsActiveTask(ScriptMovement_MoveObjects))
        ScriptMovement_StartMoveObjects(50);
    return ScriptMovement_TryAddNewMovement(GetMoveObjectsTaskId(), objEventId, movementScript);
}

bool8 ScriptMovement_IsObjectMovementFinished(u8 localId, u8 mapNum, u8 mapGroup)
{
    u8 objEventId;
    u8 taskId;
    u8 moveScrId;
    if (TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objEventId))
        return TRUE;
    taskId = GetMoveObjectsTaskId();
    moveScrId = GetMovementScriptIdFromObjectEventId(taskId, objEventId);
    if (moveScrId == OBJECT_EVENTS_COUNT)
        return TRUE;
    return IsMovementScriptFinished(taskId, moveScrId);
}

void ScriptMovement_UnfreezeObjectEvents(void)
{
    u8 taskId = GetMoveObjectsTaskId();
    if (taskId != TAIL_SENTINEL)
    {
        ScriptMovement_UnfreezeActiveObjects(taskId);
        DestroyTask(taskId);
    }
}

void ScriptMovement_StartMoveObjects(u8 priority)
{
    u8 i;
    u8 taskId = CreateTask(ScriptMovement_MoveObjects, priority);
    for (i = 1; i < NUM_TASK_DATA; i++)
    {
        gTasks[taskId].data[i] = -1;
    }
}

u8 GetMoveObjectsTaskId(void)
{
    return FindTaskIdByFunc(ScriptMovement_MoveObjects);
}

bool8 ScriptMovement_TryAddNewMovement(u8 taskId, u8 objEventId, const u8 *movementScript)
{
    u8 moveScrId;

    moveScrId = GetMovementScriptIdFromObjectEventId(taskId, objEventId);
    if (moveScrId != OBJECT_EVENTS_COUNT)
    {
        if (IsMovementScriptFinished(taskId, moveScrId) == FALSE)
        {
            return TRUE;
        }
        else
        {
            ScriptMovement_AddNewMovement(taskId, moveScrId, objEventId, movementScript);
            return FALSE;
        }
    }
    moveScrId = GetMovementScriptIdFromObjectEventId(taskId, LOCALID_PLAYER);
    if (moveScrId == OBJECT_EVENTS_COUNT)
    {
        return TRUE;
    }
    else
    {
        ScriptMovement_AddNewMovement(taskId, moveScrId, objEventId, movementScript);
        return FALSE;
    }
}

u8 GetMovementScriptIdFromObjectEventId(u8 taskId, u8 objEventId)
{
    u8 i;
    u8 *moveScriptId = (u8 *)&gTasks[taskId].data[1];
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++, moveScriptId++)
    {
        if (*moveScriptId == objEventId)
            return i;
    }
    return OBJECT_EVENTS_COUNT;
}

void LoadObjectEventIdPtrFromMovementScript(u8 taskId, u8 moveScrId, u8 **pObjEventId)
{
    u8 i;

    *pObjEventId = (u8 *)&gTasks[taskId].data[1];
    for (i = 0; i < moveScrId; i++, (*pObjEventId)++)
        ;
}

void SetObjectEventIdAtMovementScript(u8 taskId, u8 moveScrId, u8 objEventId)
{
    u8 *ptr;

    LoadObjectEventIdPtrFromMovementScript(taskId, moveScrId, &ptr);
    *ptr = objEventId;
}

void LoadObjectEventIdFromMovementScript(u8 taskId, u8 moveScrId, u8 *objEventId)
{
    u8 *ptr;

    LoadObjectEventIdPtrFromMovementScript(taskId, moveScrId, &ptr);
    *objEventId = *ptr;
}


static void ClearMovementScriptFinished(u8 taskId, u8 moveScrId)
{
    u16 mask = ~gBitTable[moveScrId];

    gTasks[taskId].data[0] &= mask;
}

static void SetMovementScriptFinished(u8 taskId, u8 moveScrId)
{
    gTasks[taskId].data[0] |= gBitTable[moveScrId];
}

static bool8 IsMovementScriptFinished(u8 taskId, u8 moveScrId)
{
    u16 moveScriptFinished = (u16)gTasks[taskId].data[0] & gBitTable[moveScrId];

    if (moveScriptFinished != 0)
        return TRUE;
    else
        return FALSE;
}

static void SetMovementScript(u8 moveScrId, const u8 *movementScript)
{
    sMovementScripts[moveScrId] = movementScript;
}

static const u8 *GetMovementScript(u8 moveScrId)
{
    return sMovementScripts[moveScrId];
}

static void ScriptMovement_AddNewMovement(u8 taskId, u8 moveScrId, u8 objEventId, const u8 *movementScript)
{
#ifdef PORTABLE
    TraceMovementScript("MoveScript: add task=%u slot=%u obj=%u script=%p first=%02X", taskId, moveScrId, objEventId, movementScript, movementScript != NULL ? movementScript[0] : 0xFF);
#endif
    ClearMovementScriptFinished(taskId, moveScrId);
    SetMovementScript(moveScrId, movementScript);
    SetObjectEventIdAtMovementScript(taskId, moveScrId, objEventId);
}

static void ScriptMovement_UnfreezeActiveObjects(u8 taskId)
{
    u8 *pObjEventId;
    u8 i;

    pObjEventId = (u8 *)&gTasks[taskId].data[1];
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++, pObjEventId++)
    {
        if (*pObjEventId != 0xFF)
            UnfreezeObjectEvent(&gObjectEvents[*pObjEventId]);
    }
}

static void ScriptMovement_MoveObjects(u8 taskId)
{
    u8 i;
    u8 objEventId;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        LoadObjectEventIdFromMovementScript(taskId, i, &objEventId);
        if (objEventId != 0xFF)
            ScriptMovement_TakeStep(taskId, i, objEventId, GetMovementScript(i));
    }
}

static void ScriptMovement_TakeStep(u8 taskId, u8 moveScrId, u8 objEventId, const u8 *movementScript)
{
    u8 nextMoveActionId;

#ifdef PORTABLE
    TraceMovementScript("MoveScript: step task=%u slot=%u obj=%u script=%p first=%02X", taskId, moveScrId, objEventId, movementScript, movementScript != NULL ? movementScript[0] : 0xFF);
#endif

    if (IsMovementScriptFinished(taskId, moveScrId) == TRUE)
    {
#ifdef PORTABLE
        TraceMovementScript("MoveScript: already-finished task=%u slot=%u obj=%u", taskId, moveScrId, objEventId);
#endif
        return;
    }
    if (ObjectEventIsHeldMovementActive(&gObjectEvents[objEventId])
        && !ObjectEventClearHeldMovementIfFinished(&gObjectEvents[objEventId]))
    {
#ifdef PORTABLE
        TraceMovementScript("MoveScript: waiting-held task=%u slot=%u obj=%u", taskId, moveScrId, objEventId);
#endif
        return;
    }

#if defined(PORTABLE)
    if (TraceMoveScriptEnvEnabled())
    {
        char _ms[128];
        snprintf(_ms, sizeof(_ms),
            "MoveScript: dispatch slot=%d obj=%d action=%02X",
            moveScrId, objEventId, movementScript[0]);
        engine_backend_trace_external(_ms);
    }
#else
    {
        char _ms[128];
        snprintf(_ms, sizeof(_ms),
            "MoveScript: dispatch slot=%d obj=%d action=%02X",
            moveScrId, objEventId, movementScript[0]);
        engine_backend_trace_external(_ms);
    }
#endif

    nextMoveActionId = *movementScript;
#ifdef PORTABLE
    TraceMovementScript("MoveScript: action slot=%u obj=%u action=%02X", moveScrId, objEventId, nextMoveActionId);
#endif
    if (nextMoveActionId == MOVEMENT_ACTION_STEP_END)
    {
#if defined(PORTABLE)
        if (TraceMoveScriptEnvEnabled())
            engine_backend_trace_external("MoveScript: action FE handler enter");
#else
        engine_backend_trace_external("MoveScript: action FE handler enter");
#endif
        SetMovementScriptFinished(taskId, moveScrId);
        FreezeObjectEvent(&gObjectEvents[objEventId]);
#if defined(PORTABLE)
        if (TraceMoveScriptEnvEnabled())
            engine_backend_trace_external("MoveScript: action FE handler exit");
#else
        engine_backend_trace_external("MoveScript: action FE handler exit");
#endif
#ifdef PORTABLE
        TraceMovementScript("MoveScript: end task=%u slot=%u obj=%u", taskId, moveScrId, objEventId);
#endif
    }
    else
    {
        if (!ObjectEventSetHeldMovement(&gObjectEvents[objEventId], nextMoveActionId))
        {
            movementScript++;
            SetMovementScript(moveScrId, movementScript);
#ifdef PORTABLE
            TraceMovementScript("MoveScript: advanced task=%u slot=%u obj=%u nextPtr=%p", taskId, moveScrId, objEventId, movementScript);
#endif
        }
#ifdef PORTABLE
        else
        {
            TraceMovementScript("MoveScript: rejected-held task=%u slot=%u obj=%u action=%02X", taskId, moveScrId, objEventId, nextMoveActionId);
        }
#endif
    }

#if defined(PORTABLE)
    if (TraceMoveScriptEnvEnabled())
        engine_backend_trace_external("MoveScript: dispatch returned");
#else
    engine_backend_trace_external("MoveScript: dispatch returned");
#endif
}
