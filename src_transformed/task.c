#include "global.h"
#include "task.h"

#ifdef PORTABLE
#include <stdarg.h>
#include <stdio.h>

extern void firered_runtime_trace_external(const char *message);
extern void BattleMainCB2(void);

static u8 sTaskTraceCount;

static void TraceTaskCreate(const char *fmt, ...)
{
    char buffer[256];
    va_list args;

    if (sTaskTraceCount >= 64)
        return;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    firered_runtime_trace_external(buffer);
    fflush(stdout);
    sTaskTraceCount++;
}
#endif

#define HEAD_SENTINEL 0xFE
#define TAIL_SENTINEL 0xFF

COMMON_DATA struct Task gTasks[NUM_TASKS] = {0};

#ifdef PORTABLE
static uintptr_t sTaskWordArgs[NUM_TASKS][NUM_TASK_DATA] = {0};
static TaskFunc sTaskFollowupFuncs[NUM_TASKS] = {0};
#endif

static void InsertTask(u8 newTaskId);
static u8 FindFirstActiveTask();

void ResetTasks(void)
{
    u8 i;

    for (i = 0; i < NUM_TASKS; i++)
    {
        gTasks[i].isActive = FALSE;
        gTasks[i].func = TaskDummy;
        gTasks[i].prev = i;
        gTasks[i].next = i + 1;
        gTasks[i].priority = -1;
        memset(gTasks[i].data, 0, sizeof(gTasks[i].data));
#ifdef PORTABLE
        memset(sTaskWordArgs[i], 0, sizeof(sTaskWordArgs[i]));
        sTaskFollowupFuncs[i] = TaskDummy;
#endif
    }

    gTasks[0].prev = HEAD_SENTINEL;
    gTasks[NUM_TASKS - 1].next = TAIL_SENTINEL;
}

u8 CreateTask(TaskFunc func, u8 priority)
{
    u8 i;

#ifdef PORTABLE
    TraceTaskCreate("TaskCreate: enter func=%p prio=%u", func, priority);
#endif

    for (i = 0; i < NUM_TASKS; i++)
    {
        if (!gTasks[i].isActive)
        {
#ifdef PORTABLE
            TraceTaskCreate("TaskCreate: slot=%u prev=%u next=%u", i, gTasks[i].prev, gTasks[i].next);
#endif
            gTasks[i].func = func;
            gTasks[i].priority = priority;
            InsertTask(i);
            memset(gTasks[i].data, 0, sizeof(gTasks[i].data));
            gTasks[i].isActive = TRUE;
#ifdef PORTABLE
            TraceTaskCreate("TaskCreate: done id=%u head=%u next=%u", i, gTasks[i].prev, gTasks[i].next);
#endif
            return i;
        }
    }

#ifdef PORTABLE
    TraceTaskCreate("TaskCreate: no free slot");
#endif
    return 0;
}

static void InsertTask(u8 newTaskId)
{
    u8 taskId = FindFirstActiveTask();

    if (taskId == NUM_TASKS)
    {
        // The new task is the only task.
        gTasks[newTaskId].prev = HEAD_SENTINEL;
        gTasks[newTaskId].next = TAIL_SENTINEL;
        return;
    }

    while (1)
    {
        if (gTasks[newTaskId].priority < gTasks[taskId].priority)
        {
            // We've found a task with a higher priority value,
            // so we insert the new task before it.
            gTasks[newTaskId].prev = gTasks[taskId].prev;
            gTasks[newTaskId].next = taskId;
            if (gTasks[taskId].prev != HEAD_SENTINEL)
                gTasks[gTasks[taskId].prev].next = newTaskId;
            gTasks[taskId].prev = newTaskId;
            return;
        }
        if (gTasks[taskId].next == TAIL_SENTINEL)
        {
            // We've reached the end.
            gTasks[newTaskId].prev = taskId;
            gTasks[newTaskId].next = gTasks[taskId].next;
            gTasks[taskId].next = newTaskId;
            return;
        }
        taskId = gTasks[taskId].next;
    }
}

void DestroyTask(u8 taskId)
{
    if (gTasks[taskId].isActive)
    {
        gTasks[taskId].isActive = FALSE;

        if (gTasks[taskId].prev == HEAD_SENTINEL)
        {
            if (gTasks[taskId].next != TAIL_SENTINEL)
                gTasks[gTasks[taskId].next].prev = HEAD_SENTINEL;
        }
        else
        {
            if (gTasks[taskId].next == TAIL_SENTINEL)
            {
                gTasks[gTasks[taskId].prev].next = TAIL_SENTINEL;
            }
            else
            {
                gTasks[gTasks[taskId].prev].next = gTasks[taskId].next;
                gTasks[gTasks[taskId].next].prev = gTasks[taskId].prev;
            }
        }
    }
}

void RunTasks(void)
{
#ifdef PORTABLE
    firered_runtime_trace_external("CrashTrace: RunTasks enter");
#endif
    u8 taskId = FindFirstActiveTask();

    if (taskId != NUM_TASKS)
    {
        do
        {
#ifdef PORTABLE
            {
                char buffer[160];

                snprintf(buffer, sizeof(buffer), "CrashTrace: task pre id=%u func=%p prev=%u next=%u pri=%u data0=%d data1=%d", taskId, gTasks[taskId].func, gTasks[taskId].prev, gTasks[taskId].next, gTasks[taskId].priority, gTasks[taskId].data[0], gTasks[taskId].data[1]);
                firered_runtime_trace_external(buffer);
            }
#endif
            gTasks[taskId].func(taskId);
#ifdef PORTABLE
            {
                char buffer[96];

                snprintf(buffer, sizeof(buffer), "CrashTrace: task post id=%u next=%u", taskId, gTasks[taskId].next);
                firered_runtime_trace_external(buffer);
            }
#endif
            taskId = gTasks[taskId].next;
        } while (taskId != TAIL_SENTINEL);
    }
}

static u8 FindFirstActiveTask()
{
    u8 taskId;

    for (taskId = 0; taskId < NUM_TASKS; taskId++)
        if (gTasks[taskId].isActive == TRUE && gTasks[taskId].prev == HEAD_SENTINEL)
            break;

    return taskId;
}

void TaskDummy(u8 taskId)
{
}

void SetTaskFuncWithFollowupFunc(u8 taskId, TaskFunc func, TaskFunc followupFunc)
{
    u8 followupFuncIndex = NUM_TASK_DATA - 2; // Should be const.

#ifdef PORTABLE
    sTaskFollowupFuncs[taskId] = followupFunc;
#endif
    gTasks[taskId].data[followupFuncIndex] = (s16)((u32)followupFunc);
    gTasks[taskId].data[followupFuncIndex + 1] = (s16)((u32)followupFunc >> 16); // Store followupFunc as two half-words in the data array.
    gTasks[taskId].func = func;
}

void SwitchTaskToFollowupFunc(u8 taskId)
{
    u8 followupFuncIndex = NUM_TASK_DATA - 2; // Should be const.

#ifdef PORTABLE
    gTasks[taskId].func = sTaskFollowupFuncs[taskId];
#else
    gTasks[taskId].func = (TaskFunc)((u16)(gTasks[taskId].data[followupFuncIndex]) | (gTasks[taskId].data[followupFuncIndex + 1] << 16));
#endif
}

bool8 FuncIsActiveTask(TaskFunc func)
{
    u8 i;

    for (i = 0; i < NUM_TASKS; i++)
        if (gTasks[i].isActive == TRUE && gTasks[i].func == func)
            return TRUE;

    return FALSE;
}

u8 FindTaskIdByFunc(TaskFunc func)
{
    s32 i;

    for (i = 0; i < NUM_TASKS; i++)
        if (gTasks[i].isActive == TRUE && gTasks[i].func == func)
            return (u8)i;

    return -1;
}

u8 GetTaskCount(void)
{
    u8 i;
    u8 count = 0;

    for (i = 0; i < NUM_TASKS; i++)
        if (gTasks[i].isActive == TRUE)
            count++;

    return count;
}

void SetWordTaskArg(u8 taskId, u8 dataElem, uintptr_t value)
{
    if (dataElem <= 14)
    {
        u32 lowValue = (u32)value;
#ifdef PORTABLE
        sTaskWordArgs[taskId][dataElem] = value;
#endif
        gTasks[taskId].data[dataElem] = (s16)(lowValue & 0xFFFF);
        gTasks[taskId].data[dataElem + 1] = (s16)(lowValue >> 16);
    }
}

uintptr_t GetWordTaskArg(u8 taskId, u8 dataElem)
{
    if (dataElem <= 14)
#ifdef PORTABLE
        return sTaskWordArgs[taskId][dataElem];
#else
        return (u16)gTasks[taskId].data[dataElem] | (gTasks[taskId].data[dataElem + 1] << 16);
#endif
    else
        return 0;
}
