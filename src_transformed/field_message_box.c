#include "global.h"
#include "field_message_box.h"
#include "gflib.h"
#include "new_menu_helpers.h"
#include "quest_log.h"
#include "script.h"
#include "text_window.h"

#ifdef PORTABLE
#include <stdarg.h>
#include <stdio.h>

extern void firered_runtime_trace_external(const char *message);

static void TraceFieldMessage(const char *fmt, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    firered_runtime_trace_external(buffer);
}

static void TraceFieldMessageBytes(const char *label, const u8 *str)
{
    char buffer[256];

    if (str == NULL)
        return;

    snprintf(buffer, sizeof(buffer),
             "%s %u %u %u %u %u %u %u %u %u %u %u %u",
             label,
             str[0], str[1], str[2], str[3], str[4], str[5],
             str[6], str[7], str[8], str[9], str[10], str[11]);
    firered_runtime_trace_external(buffer);
}
#endif

static EWRAM_DATA u8 sMessageBoxType = 0;

static void ExpandStringAndStartDrawFieldMessageBox(const u8 *str);
static void StartDrawFieldMessageBox(void);

void InitFieldMessageBox(void)
{
    sMessageBoxType = FIELD_MESSAGE_BOX_HIDDEN;
    gTextFlags.canABSpeedUpPrint = FALSE;
    gTextFlags.useAlternateDownArrow = FALSE;
    gTextFlags.autoScroll = FALSE;
}

static void Task_DrawFieldMessageBox(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    switch (task->data[0])
    {
    case 0:
#ifdef PORTABLE
        TraceFieldMessage("FieldMsgTask: case0 ql=%d sign=%d", gQuestLogState, IsMsgSignpost());
#endif
        if (gQuestLogState == QL_STATE_PLAYBACK)
        {
            gTextFlags.autoScroll = TRUE;
            LoadQuestLogWindowTiles(0, 0x200);
        }
        else if (!IsMsgSignpost())
            LoadStdWindowFrameGfx();
        else
            LoadSignpostWindowFrameGfx();
        task->data[0]++;
        break;
    case 1:
#ifdef PORTABLE
        TraceFieldMessage("FieldMsgTask: case1 draw frame");
#endif
        DrawDialogueFrame(0, TRUE);
        task->data[0]++;
        break;
    case 2:
#ifdef PORTABLE
        TraceFieldMessage("FieldMsgTask: case2 printerActive=%d", RunTextPrinters_CheckPrinter0Active());
#endif
        if (RunTextPrinters_CheckPrinter0Active() != TRUE)
        {
            sMessageBoxType = FIELD_MESSAGE_BOX_HIDDEN;
            DestroyTask(taskId);
        }
        break;
    }
}

static void CreateTask_DrawFieldMessageBox(void)
{
#ifdef PORTABLE
    TraceFieldMessage("FieldMsg: pre-CreateTask");
#endif
    CreateTask(Task_DrawFieldMessageBox, 80);
#ifdef PORTABLE
    TraceFieldMessage("FieldMsg: post-CreateTask");
#endif
}

static void DestroyTask_DrawFieldMessageBox(void)
{
    u8 taskId = FindTaskIdByFunc(Task_DrawFieldMessageBox);
    if (taskId != 0xFF)
        DestroyTask(taskId);
}

bool8 ShowFieldMessage(const u8 *str)
{
#ifdef PORTABLE
    TraceFieldMessage("FieldMsg: show type=%u str=%p b0=%u b1=%u b2=%u b3=%u", sMessageBoxType, str, str ? str[0] : 0, str ? str[1] : 0, str ? str[2] : 0, str ? str[3] : 0);
#endif
    if (sMessageBoxType != FIELD_MESSAGE_BOX_HIDDEN)
        return FALSE;
    ExpandStringAndStartDrawFieldMessageBox(str);
    sMessageBoxType = FIELD_MESSAGE_BOX_NORMAL;
    return TRUE;
}

bool8 ShowFieldAutoScrollMessage(const u8 *str)
{
    if (sMessageBoxType != FIELD_MESSAGE_BOX_HIDDEN)
        return FALSE;
    sMessageBoxType = FIELD_MESSAGE_BOX_AUTO_SCROLL;
    ExpandStringAndStartDrawFieldMessageBox(str);
    return TRUE;
}

// Unused
static bool8 ForceShowFieldAutoScrollMessage(const u8 *str)
{
    sMessageBoxType = FIELD_MESSAGE_BOX_AUTO_SCROLL;
    ExpandStringAndStartDrawFieldMessageBox(str);
    return TRUE;
}

// Unused
// Same as ShowFieldMessage, but instead of accepting a string argument,
// it just prints whatever that's already in gStringVar4
static bool8 ShowFieldMessageFromBuffer(void)
{
    if (sMessageBoxType != FIELD_MESSAGE_BOX_HIDDEN)
        return FALSE;
    sMessageBoxType = FIELD_MESSAGE_BOX_NORMAL;
    StartDrawFieldMessageBox();
    return TRUE;
}

static void ExpandStringAndStartDrawFieldMessageBox(const u8 *str)
{
#ifdef PORTABLE
    TraceFieldMessage("FieldMsg: expand start str=%p", str);
#endif
    StringExpandPlaceholders(gStringVar4, str);
#ifdef PORTABLE
    TraceFieldMessage("FieldMsg: expand done out0=%u out1=%u out2=%u out3=%u", gStringVar4[0], gStringVar4[1], gStringVar4[2], gStringVar4[3]);
    TraceFieldMessageBytes("FieldMsg: expand bytes", gStringVar4);
#endif
    AddTextPrinterDiffStyle(TRUE);
#ifdef PORTABLE
    TraceFieldMessage("FieldMsg: printer started");
    TraceFieldMessage("FieldMsg: direct pre-CreateTask");
#endif
    CreateTask(Task_DrawFieldMessageBox, 80);
#ifdef PORTABLE
    TraceFieldMessage("FieldMsg: direct post-CreateTask");
#endif
}

static void StartDrawFieldMessageBox(void)
{
    AddTextPrinterDiffStyle(TRUE);
    CreateTask_DrawFieldMessageBox();
}

void HideFieldMessageBox(void)
{
    DestroyTask_DrawFieldMessageBox();
    ClearDialogWindowAndFrame(0, TRUE);
    sMessageBoxType = FIELD_MESSAGE_BOX_HIDDEN;
}

u8 GetFieldMessageBoxType(void)
{
    return sMessageBoxType;
}

bool8 IsFieldMessageBoxHidden(void)
{
    if (sMessageBoxType == FIELD_MESSAGE_BOX_HIDDEN)
        return TRUE;
    else
        return FALSE;
}

// Unused
static void ReplaceFieldMessageWithFrame(void)
{
    DestroyTask_DrawFieldMessageBox();
    DrawStdWindowFrame(0, TRUE);
    sMessageBoxType = FIELD_MESSAGE_BOX_HIDDEN;
}
