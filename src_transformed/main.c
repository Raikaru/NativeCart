#include "global.h"
#include "gflib.h"
#include "link.h"
#include "link_rfu.h"
#include "load_save.h"
#include "m4a.h"
#include "random.h"
#include "gba/flash_internal.h"
#include "help_system.h"
#include "new_menu_helpers.h"
#include "overworld.h"
#include "play_time.h"
#include "intro.h"
#include "task.h"
#include "battle_controllers.h"
#include "scanline_effect.h"
#include "save_failed_screen.h"
#include "quest_log.h"

#ifdef PORTABLE
#include <stdio.h>
#include <stdarg.h>

extern void firered_runtime_trace_external(const char *message);

static void TraceMainLoop(const char *fmt, ...)
{
    char buffer[160];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    firered_runtime_trace_external(buffer);
}
#endif

#ifdef PORTABLE
extern MainCallback firered_portable_get_cb2_overworld(void);
#endif

extern u32 intr_main[];

static void VBlankIntr(void);
static void HBlankIntr(void);
static void VCountIntr(void);
static void SerialIntr(void);
static void IntrDummy(void);

const u8 gGameVersion = GAME_VERSION;

const u8 gGameLanguage = GAME_LANGUAGE;

#if MODERN
const char BuildDateTime[] = __DATE__ " " __TIME__;
#else
#if REVISION == 0
const char BuildDateTime[] = "2004 04 26 11:20";
#else
const char BuildDateTime[] = "2004 07 20 09:30";
#endif //REVISION
#endif //MODERN

const IntrFunc gIntrTableTemplate[] =
{
    VCountIntr, // V-count interrupt
    SerialIntr, // Serial interrupt
    Timer3Intr, // Timer 3 interrupt
    HBlankIntr, // H-blank interrupt
    VBlankIntr, // V-blank interrupt
    IntrDummy,  // Timer 0 interrupt
    IntrDummy,  // Timer 1 interrupt
    IntrDummy,  // Timer 2 interrupt
    IntrDummy,  // DMA 0 interrupt
    IntrDummy,  // DMA 1 interrupt
    IntrDummy,  // DMA 2 interrupt
    IntrDummy,  // DMA 3 interrupt
    IntrDummy,  // Key interrupt
    IntrDummy,  // Game Pak interrupt
};

#define INTR_COUNT ((int)(sizeof(gIntrTableTemplate)/sizeof(IntrFunc)))

COMMON_DATA u16 gKeyRepeatStartDelay = 0;
COMMON_DATA u8 gLinkTransferringData = 0;
COMMON_DATA struct Main gMain = {0};
COMMON_DATA u16 gKeyRepeatContinueDelay = 0;
COMMON_DATA u8 gSoftResetDisabled = 0;
COMMON_DATA IntrFunc gIntrTable[INTR_COUNT] = {0};
COMMON_DATA u8 sVcountAfterSound = 0;
COMMON_DATA bool8 gLinkVSyncDisabled = 0;
COMMON_DATA u32 IntrMain_Buffer[0x200] = {0};
COMMON_DATA u8 sVcountAtIntr = 0;
COMMON_DATA u8 sVcountBeforeSound = 0;
COMMON_DATA u8 gPcmDmaCounter = 0;

static IntrFunc * const sTimerIntrFunc = gIntrTable + 0x7;

EWRAM_DATA u8 gDecompressionBuffer[0x4000] = {0};
EWRAM_DATA u16 gTrainerId = 0;

static void UpdateLinkAndCallCallbacks(void);
static void InitMainCallbacks(void);
static void CallCallbacks(void);
static void ReadKeys(void);
void InitIntrHandlers(void);
static void WaitForVBlank(void);
void EnableVCountIntrAtLine150(void);

#define B_START_SELECT (B_BUTTON | START_BUTTON | SELECT_BUTTON)

void AgbMain()
{
#ifdef PORTABLE
    firered_runtime_trace_external("AgbMain: start");
#endif
#if MODERN
    // Modern compilers are liberal with the stack on entry to this function,
    // so RegisterRamReset may crash if it resets IWRAM.
    RegisterRamReset(RESET_ALL & ~RESET_IWRAM);
    asm("mov\tr1, #0xC0\n"
        "\tlsl\tr1, r1, #0x12\n"
        "\tmov\tr2, #0xFC\n"
        "\tlsl\tr2, r2, #0x7\n"
        "\tadd\tr2, r1, r2\n"
        "\tmov\tr0, #0\n"
        "\tmov\tr3, r0\n"
        "\tmov\tr4, r0\n"
        "\tmov\tr5, r0\n"
        ".LCU%=:\n"
        "\tstmia\tr1!, {r0, r3, r4, r5}\n"
        "\tstmia\tr1!, {r0, r3, r4, r5}\n"
        "\tstmia\tr1!, {r0, r3, r4, r5}\n"
        "\tstmia\tr1!, {r0, r3, r4, r5}\n"
        "\tstmia\tr1!, {r0, r3, r4, r5}\n"
        "\tstmia\tr1!, {r0, r3, r4, r5}\n"
        "\tstmia\tr1!, {r0, r3, r4, r5}\n"
        "\tstmia\tr1!, {r0, r3, r4, r5}\n"
        "\tcmp\tr1, r2\n"
        "\tbcc\t.LCU%=\n"
        :
        :
        : "r0", "r1", "r2", "r3", "r4", "r5", "memory"
    );
#else
    RegisterRamReset(RESET_ALL);
#endif //MODERN
    *(vu16 *)BG_PLTT = RGB_WHITE;
#ifdef PORTABLE
    firered_runtime_trace_external("AgbMain: pre-InitGpuRegManager");
#endif
    InitGpuRegManager();
    REG_WAITCNT = WAITCNT_PREFETCH_ENABLE | WAITCNT_WS0_S_1 | WAITCNT_WS0_N_3;
    InitKeys();
#ifdef PORTABLE
    firered_runtime_trace_external("AgbMain: pre-InitIntrHandlers");
#endif
    InitIntrHandlers();
#ifdef PORTABLE
    firered_runtime_trace_external("AgbMain: post-InitIntrHandlers");
#endif
    m4aSoundInit();
    EnableVCountIntrAtLine150();
#ifndef PORTABLE
    InitRFU();
#endif
    CheckForFlashMemory();
#ifdef PORTABLE
    firered_runtime_trace_external("AgbMain: pre-InitMainCallbacks");
#endif
    InitMainCallbacks();
#ifdef PORTABLE
    firered_runtime_trace_external("AgbMain: post-InitMainCallbacks");
#endif
    InitMapMusic();
    ClearDma3Requests();
    ResetBgs();
    InitHeap(gHeap, HEAP_SIZE);
    SetDefaultFontsPointer();

    gSoftResetDisabled = FALSE;
    gHelpSystemEnabled = FALSE;

    SetNotInSaveFailedScreen();

#ifndef NDEBUG
#if (LOG_HANDLER == LOG_HANDLER_MGBA_PRINT)
    (void) MgbaOpen();
#elif (LOG_HANDLER == LOG_HANDLER_AGB_PRINT)
    AGBPrintInit();
#endif
#endif

#if REVISION == 1
    if (gFlashMemoryPresent != TRUE)
        SetMainCallback2(NULL);
#endif

    gLinkTransferringData = FALSE;

    for (;;)
    {
#ifdef PORTABLE
        volatile u32 stackCanary = 0xDEADBEEF;
#endif
        ReadKeys();
#ifdef PORTABLE
        firered_runtime_trace_external("MainLoop: post-vblank B");
#endif

        if (
#ifndef PORTABLE
            gSoftResetDisabled == FALSE
         && (gMain.heldKeysRaw & A_BUTTON)
         && (gMain.heldKeysRaw & B_START_SELECT) == B_START_SELECT)
#else
            FALSE)
#endif
        {
            rfu_REQ_stopMode();
            rfu_waitREQComplete();
            DoSoftReset();
        }
#ifdef PORTABLE
        firered_runtime_trace_external("MainLoop: post-vblank C");
#endif

        if (Overworld_SendKeysToLinkIsRunning() == TRUE)
        {
            gLinkTransferringData = TRUE;
            UpdateLinkAndCallCallbacks();
            gLinkTransferringData = FALSE;
        }
        else
        {
            gLinkTransferringData = FALSE;
            UpdateLinkAndCallCallbacks();
#ifdef PORTABLE
            if (gMain.callback2 == CB2_InitBattle)
                firered_runtime_trace_external("MainLoop: post-UpdateLinkAndCallCallbacks with CB2_InitBattle");
#endif

            if (Overworld_RecvKeysFromLinkIsRunning() == 1)
            {
                gMain.newKeys = 0;
                ClearSpriteCopyRequests();
                gLinkTransferringData = TRUE;
                UpdateLinkAndCallCallbacks();
                gLinkTransferringData = FALSE;
            }
        }
#ifdef PORTABLE
        firered_runtime_trace_external("MainLoop: post-vblank D");
#endif

        PlayTimeCounter_Update();
        MapMusicMain();
#ifdef PORTABLE
        {
            int t;

            for (t = 0; t < NUM_TASKS; t++)
                if (gTasks[t].isActive)
                    TraceMainLoop("ActiveTask: id=%d func=%p", t, gTasks[t].func);
        }
        if (gMain.callback2 == CB2_InitBattle)
            firered_runtime_trace_external("MainLoop: pre-WaitForVBlank with CB2_InitBattle");
#endif
        WaitForVBlank();
#ifdef PORTABLE
        if (stackCanary != 0xDEADBEEF)
        {
            TraceMainLoop("STACK CORRUPT: canary=%08X frame=%u cb1=%p activeBattler=%d",
                stackCanary, *gMain.vblankCounter1, gMain.callback1, gActiveBattler);
        }
        firered_runtime_trace_external("MainLoop: post-vblank A");
#endif
#ifdef PORTABLE
        if (gMain.callback2 == CB2_InitBattle)
            firered_runtime_trace_external("MainLoop: post-WaitForVBlank with CB2_InitBattle");
#endif
    }
}

static void UpdateLinkAndCallCallbacks(void)
{
#ifdef PORTABLE
    firered_runtime_trace_external("MainLoop: post-vblank E");
    CallCallbacks();
#else
    if (!HandleLinkConnection())
        CallCallbacks();
#endif
}

static void InitMainCallbacks(void)
{
    gMain.vblankCounter1 = 0;
    gMain.vblankCounter2 = 0;
    gMain.callback1 = NULL;
    SetMainCallback2(CB2_InitCopyrightScreenAfterBootup);
    gSaveBlock2Ptr = &gSaveBlock2;
    gSaveBlock1Ptr = &gSaveBlock1;
    gPokemonStoragePtr = &gPokemonStorage;
    gSaveBlock2.encryptionKey = 0;
    gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
}

static void CallCallbacks(void)
{
    bool32 saveFailed = RunSaveFailedScreen();
#ifdef PORTABLE
    firered_runtime_trace_external("MainLoop: post-vblank F");
#endif
    u8 helpSystem = RunHelpSystemCallback();
#ifdef PORTABLE
    firered_runtime_trace_external("MainLoop: post-vblank G");
#endif

    if (!saveFailed && !helpSystem)
    {
        if (gMain.callback1)
        {
#ifdef PORTABLE
            TraceMainLoop("MainLoop: pre-cb1 dispatch cb1=%p", gMain.callback1);
#endif
            gMain.callback1();
#ifdef PORTABLE
            firered_runtime_trace_external("MainLoop: post-cb1 dispatch");
#endif
        }

        if (gMain.callback2)
        {
#ifdef PORTABLE
            if (gMain.callback2 == BattleMainCB2)
                firered_runtime_trace_external("MainLoop: pre-BattleMainCB2 call");
#endif
            gMain.callback2();
#ifdef PORTABLE
            if (gMain.callback2 == BattleMainCB2)
                firered_runtime_trace_external("MainLoop: post-BattleMainCB2 call");
#endif
        }
    }
}

void SetMainCallback2(MainCallback callback)
{
    gMain.callback2 = callback;
    gMain.state = 0;
}

void StartTimer1(void)
{
    REG_TM1CNT_H = 0x80;
}

void SeedRngAndSetTrainerId(void)
{
    u16 val = REG_TM1CNT_L;
    SeedRng(val);
    REG_TM1CNT_H = 0;
    gTrainerId = val;
}

u16 GetGeneratedTrainerIdLower(void)
{
    return gTrainerId;
}

void EnableVCountIntrAtLine150(void)
{
    u16 gpuReg = (GetGpuReg(REG_OFFSET_DISPSTAT) & 0xFF) | (150 << 8);
    SetGpuReg(REG_OFFSET_DISPSTAT, gpuReg | DISPSTAT_VCOUNT_INTR);
    EnableInterrupts(INTR_FLAG_VCOUNT);
}

void InitKeys(void)
{
    gKeyRepeatContinueDelay = 5;
    gKeyRepeatStartDelay = 40;

    gMain.heldKeys = 0;
    gMain.newKeys = 0;
    gMain.newAndRepeatedKeys = 0;
    gMain.heldKeysRaw = 0;
    gMain.newKeysRaw = 0;
}

static void ReadKeys(void)
{
    u16 keyInput = REG_KEYINPUT ^ KEYS_MASK;
    gMain.newKeysRaw = keyInput & ~gMain.heldKeysRaw;
    gMain.newKeys = gMain.newKeysRaw;
    gMain.newAndRepeatedKeys = gMain.newKeysRaw;

    // BUG: Key repeat won't work when pressing L using L=A button mode
    // because it compares the raw key input with the remapped held keys.
    // Note that newAndRepeatedKeys is never remapped either.

    if (keyInput != 0 && gMain.heldKeys == keyInput)
    {
        gMain.keyRepeatCounter--;

        if (gMain.keyRepeatCounter == 0)
        {
            gMain.newAndRepeatedKeys = keyInput;
            gMain.keyRepeatCounter = gKeyRepeatContinueDelay;
        }
    }
    else
    {
        // If there is no input or the input has changed, reset the counter.
        gMain.keyRepeatCounter = gKeyRepeatStartDelay;
    }

    gMain.heldKeysRaw = keyInput;
    gMain.heldKeys = gMain.heldKeysRaw;

    // Remap L to A if the L=A option is enabled.
    if (gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
    {
        if (JOY_NEW(L_BUTTON))
            gMain.newKeys |= A_BUTTON;

        if (JOY_HELD(L_BUTTON))
            gMain.heldKeys |= A_BUTTON;
    }

    if (JOY_NEW(gMain.watchedKeysMask))
        gMain.watchedKeysPressed = TRUE;
}

void InitIntrHandlers(void)
{
    int i;

#ifdef PORTABLE
    firered_runtime_trace_external("InitIntrHandlers: start");
#endif
    for (i = 0; i < INTR_COUNT; i++)
        gIntrTable[i] = gIntrTableTemplate[i];

#ifdef PORTABLE
    firered_runtime_trace_external("InitIntrHandlers: post-table-copy");
    firered_runtime_trace_external("InitIntrHandlers: pre-DMA intr_main");
#endif

    DmaCopy32(3, intr_main, IntrMain_Buffer, sizeof(IntrMain_Buffer));

#ifdef PORTABLE
    firered_runtime_trace_external("InitIntrHandlers: post-DMA intr_main");
    firered_runtime_trace_external("InitIntrHandlers: pre-INTR_VECTOR");
#endif

#ifndef PORTABLE
    INTR_VECTOR = IntrMain_Buffer;
#else
    firered_runtime_trace_external("InitIntrHandlers: skip-INTR_VECTOR");
#endif

#ifdef PORTABLE
    firered_runtime_trace_external("InitIntrHandlers: post-INTR_VECTOR");
    firered_runtime_trace_external("InitIntrHandlers: pre-callback-clears");
#endif

    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    SetSerialCallback(NULL);

#ifdef PORTABLE
    firered_runtime_trace_external("InitIntrHandlers: post-callback-clears");
    firered_runtime_trace_external("InitIntrHandlers: pre-REG_IME");
#endif

    REG_IME = 1;

#ifdef PORTABLE
    firered_runtime_trace_external("InitIntrHandlers: post-REG_IME");
    firered_runtime_trace_external("InitIntrHandlers: pre-EnableInterrupts");
#endif

    EnableInterrupts(INTR_FLAG_VBLANK);

#ifdef PORTABLE
    firered_runtime_trace_external("InitIntrHandlers: post-EnableInterrupts");
    firered_runtime_trace_external("InitIntrHandlers: exit");
#endif
}

void SetVBlankCallback(IntrCallback callback)
{
    gMain.vblankCallback = callback;
}

void SetHBlankCallback(IntrCallback callback)
{
    gMain.hblankCallback = callback;
}

void SetVCountCallback(IntrCallback callback)
{
    gMain.vcountCallback = callback;
}

void SetSerialCallback(IntrCallback callback)
{
    gMain.serialCallback = callback;
}

extern void CopyBufferedValuesToGpuRegs(void);
extern void ProcessDma3Requests(void);

static void VBlankIntr(void)
{
#ifdef PORTABLE
    if (gMain.callback2 == CB2_InitBattle)
        firered_runtime_trace_external("VBlankIntr: enter battle handoff");
#endif
    if (gWirelessCommType)
        RfuVSync();
    else if (!gLinkVSyncDisabled)
        LinkVSync();

    if (gMain.vblankCounter1)
        (*gMain.vblankCounter1)++;

    if (gMain.vblankCallback)
    {
#ifdef PORTABLE
        if (gMain.callback2 == CB2_InitBattle)
            firered_runtime_trace_external("VBlankIntr: pre-vblankCallback");
#endif
        gMain.vblankCallback();
    }

    gMain.vblankCounter2++;

    CopyBufferedValuesToGpuRegs();
    ProcessDma3Requests();
#ifdef PORTABLE
    if (gMain.callback2 == CB2_InitBattle)
        firered_runtime_trace_external("VBlankIntr: post-dma battle handoff");
#endif

    gPcmDmaCounter = gSoundInfo.pcmDmaCounter;

#ifndef NDEBUG
    sVcountBeforeSound = REG_VCOUNT;
#endif
    m4aSoundMain();
#ifdef PORTABLE
    if (gMain.callback2 == CB2_InitBattle)
        firered_runtime_trace_external("VBlankIntr: exit battle handoff");
#endif
#ifndef NDEBUG
    sVcountAfterSound = REG_VCOUNT;
#endif

    TryReceiveLinkBattleData();
    Random();
    UpdateWirelessStatusIndicatorSprite();

    INTR_CHECK |= INTR_FLAG_VBLANK;
    gMain.intrCheck |= INTR_FLAG_VBLANK;
}

void InitFlashTimer(void)
{
    IntrFunc **func = (IntrFunc **)&sTimerIntrFunc;
    SetFlashTimerIntr(2, *func);
}

static void HBlankIntr(void)
{
    if (gMain.hblankCallback)
        gMain.hblankCallback();

    INTR_CHECK |= INTR_FLAG_HBLANK;
    gMain.intrCheck |= INTR_FLAG_HBLANK;
}

static void VCountIntr(void)
{
#ifndef NDEBUG
    sVcountAtIntr = REG_VCOUNT;
#endif
    m4aSoundVSync();
    INTR_CHECK |= INTR_FLAG_VCOUNT;
    gMain.intrCheck |= INTR_FLAG_VCOUNT;
}

static void SerialIntr(void)
{
    if (gMain.serialCallback)
        gMain.serialCallback();

    INTR_CHECK |= INTR_FLAG_SERIAL;
    gMain.intrCheck |= INTR_FLAG_SERIAL;
}

void RestoreSerialTimer3IntrHandlers(void)
{
    gIntrTable[1] = SerialIntr;
    gIntrTable[2] = Timer3Intr;
}

static void IntrDummy(void)
{}

static void WaitForVBlank(void)
{
#ifdef PORTABLE
    char buffer[128];
    volatile u32 vblankCanary = 0xCAFEBABE;

    snprintf(buffer, sizeof(buffer), "WaitForVBlank: enter cb2=%p vblank=%p", gMain.callback2, gMain.vblankCallback);
    firered_runtime_trace_external(buffer);
    if (gMain.vblankCallback)
        firered_runtime_trace_external("WaitForVBlank: pre-VBlankIntrWait has callback");
    else
        firered_runtime_trace_external("WaitForVBlank: pre-VBlankIntrWait no callback");
    VBlankIntrWait();
    if (vblankCanary != 0xCAFEBABE)
    {
        TraceMainLoop("STACK CORRUPT IN VBLANK: canary=%08X frame=%u",
            vblankCanary, *gMain.vblankCounter1);
    }
    firered_runtime_trace_external("WaitForVBlank: exit");
    TraceMainLoop("MainLoop: post-vblank pre-cb1 cb1=%p", gMain.callback1);
#else
    gMain.intrCheck &= ~INTR_FLAG_VBLANK;

    while (!(gMain.intrCheck & INTR_FLAG_VBLANK))
        ;
#endif
}

void SetVBlankCounter1Ptr(u32 *ptr)
{
    gMain.vblankCounter1 = ptr;
}

void DisableVBlankCounter1(void)
{
    gMain.vblankCounter1 = NULL;
}

void DoSoftReset(void)
{
    REG_IME = 0;
    m4aSoundVSyncOff();
    ScanlineEffect_Stop();
    DmaStop(1);
    DmaStop(2);
    DmaStop(3);
    SoftReset(RESET_ALL & ~RESET_SIO_REGS);
}

void ClearPokemonCrySongs(void)
{
    CpuFill16(0, gPokemonCrySongs, MAX_POKEMON_CRIES * sizeof(struct PokemonCrySong));
}
