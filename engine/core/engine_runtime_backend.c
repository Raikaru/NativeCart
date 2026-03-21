#include "engine_internal.h"
#include "engine_backend.h"
#include "engine_runtime_internal.h"
#include "../../include/main.h"
#include "../../include/overworld.h"
#include "../../include/palette.h"
#include "../../include/pokemon.h"
#include "../../include/pokemon_storage_system.h"
#include "../../include/gpu_regs.h"
#include "../../include/script.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#endif

#define ENGINE_TRACE_RING_SIZE 2048
#define ENGINE_TRACE_LINE_MAX 512

#define ENGINE_TRACE_CAT_CRASH    (1u << 0)
#define ENGINE_TRACE_CAT_BATTLE   (1u << 1)
#define ENGINE_TRACE_CAT_TASK     (1u << 2)
#define ENGINE_TRACE_CAT_MOVE     (1u << 3)
#define ENGINE_TRACE_CAT_WEATHER  (1u << 4)
#define ENGINE_TRACE_CAT_RENDERER (1u << 5)
#define ENGINE_TRACE_CAT_MAINLOOP (1u << 6)
#define ENGINE_TRACE_CAT_WARN     (1u << 7)
#define ENGINE_TRACE_CAT_GENERAL  (1u << 8)

#define ENGINE_TRACE_DEFAULT_MASK (ENGINE_TRACE_CAT_CRASH | ENGINE_TRACE_CAT_BATTLE | ENGINE_TRACE_CAT_TASK | ENGINE_TRACE_CAT_MOVE | ENGINE_TRACE_CAT_WARN)

typedef struct EngineTraceEntry {
    char line[ENGINE_TRACE_LINE_MAX];
} EngineTraceEntry;

#ifdef _WIN32
typedef LONG EngineTraceAtomicInt;
#define engine_trace_atomic_load(ptr) InterlockedCompareExchange((volatile LONG *)(ptr), 0, 0)
#define engine_trace_atomic_compare_exchange(ptr, expected, desired) \
    (InterlockedCompareExchange((volatile LONG *)(ptr), (LONG)(desired), (LONG)(expected)) == (LONG)(expected))
#define engine_trace_atomic_increment(ptr) InterlockedIncrement((volatile LONG *)(ptr))
#define engine_trace_atomic_exchange(ptr, value) InterlockedExchange((volatile LONG *)(ptr), (LONG)(value))
#define engine_trace_atomic_store(ptr, value) InterlockedExchange((volatile LONG *)(ptr), (LONG)(value))
#else
typedef int EngineTraceAtomicInt;
#define engine_trace_atomic_load(ptr) __atomic_load_n((ptr), __ATOMIC_ACQUIRE)
#define engine_trace_atomic_compare_exchange(ptr, expected, desired) \
    __atomic_compare_exchange_n((ptr), &(expected), (desired), 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)
#define engine_trace_atomic_increment(ptr) __atomic_add_fetch((ptr), 1, __ATOMIC_ACQ_REL)
#define engine_trace_atomic_exchange(ptr, value) __atomic_exchange_n((ptr), (value), __ATOMIC_ACQ_REL)
#define engine_trace_atomic_store(ptr, value) __atomic_store_n((ptr), (value), __ATOMIC_RELEASE)
#endif

static EngineTraceEntry s_trace_ring[ENGINE_TRACE_RING_SIZE];
static volatile EngineTraceAtomicInt s_trace_ready[ENGINE_TRACE_RING_SIZE];
static volatile EngineTraceAtomicInt s_trace_head = 0;
static volatile EngineTraceAtomicInt s_trace_tail = 0;
static volatile EngineTraceAtomicInt s_trace_dropped = 0;
static uint64_t s_trace_last_flush_ms = 0;
/* Off by default: set NATIVECART_TRACE (e.g. "default", "battle", "all") to enable. */
static uint32_t s_trace_mask = 0u;
static int s_trace_config_initialized = 0;

extern void AgbMain(void);
extern void CopyBufferedValuesToGpuRegs(void);
extern void ProcessDma3Requests(void);
extern void CB2_NewGame(void);
extern void CB2_InitBattle(void);
extern void (*gFieldCallback)(void);
extern bool8 (*gFieldCallback2)(void);
#ifdef PORTABLE
extern MainCallback firered_portable_get_cb2_overworld(void);
extern MainCallback firered_portable_get_cb2_overworld_basic(void);
extern void firered_portable_init_map_object_event_script_words(void);
#endif

uint32_t intr_main[0x200];

typedef struct EngineRuntimeState {
    uint8_t *rom_copy;
    size_t rom_size;
    int initialized;
    int shutdown_requested;
    int soft_reset_requested;
    unsigned long requested_frames;
    unsigned long completed_frames;
#ifdef _WIN32
    HANDLE thread;
    CRITICAL_SECTION mutex;
    CONDITION_VARIABLE cond;
#else
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
} EngineRuntimeState;

static EngineRuntimeState g_runtime;

typedef struct EngineStateFileHeader {
    char magic[8];
    uint32_t version;
    uint32_t ewram_size;
    uint32_t iwram_size;
    uint32_t ioreg_size;
    uint32_t palette_size;
    uint32_t vram_size;
    uint32_t oam_size;
    uint32_t sram_size;
} EngineStateFileHeader;

#define ENGINE_STATE_FILE_VERSION 2u

static void engine_runtime_trace(const char *message);
static void engine_runtime_trace_flush(void);
static int engine_runtime_any_trace_enabled(void);
static int engine_runtime_begin_idle_mutation(void);
static void engine_runtime_end_idle_mutation(void);

static int engine_trace_starts_with(const char *message, const char *prefix)
{
    while (*prefix != '\0')
    {
        if (*message != *prefix)
            return 0;
        ++message;
        ++prefix;
    }
    return 1;
}

static int engine_trace_token_equals(const char *start, size_t length, const char *token)
{
    size_t i;

    for (i = 0; i < length; ++i)
    {
        if (token[i] == '\0' || start[i] != token[i])
            return 0;
    }

    return token[length] == '\0';
}

static uint32_t engine_runtime_trace_category_for_message(const char *message)
{
    if (message == NULL || *message == '\0')
        return ENGINE_TRACE_CAT_GENERAL;
    if (engine_trace_starts_with(message, "WARN:"))
        return ENGINE_TRACE_CAT_WARN;
    if (engine_trace_starts_with(message, "CrashTrace:"))
        return ENGINE_TRACE_CAT_CRASH;
    if (engine_trace_starts_with(message, "BattleMain")
     || engine_trace_starts_with(message, "BattleMainCB1:")
     || engine_trace_starts_with(message, "UpdatePaletteFade:"))
        return ENGINE_TRACE_CAT_BATTLE;
    if (engine_trace_starts_with(message, "PerStepCB:")
     || engine_trace_starts_with(message, "RunTasks:"))
        return ENGINE_TRACE_CAT_TASK;
    if (engine_trace_starts_with(message, "MoveScript:"))
        return ENGINE_TRACE_CAT_MOVE;
    if (engine_trace_starts_with(message, "WeatherMain:")
     || engine_trace_starts_with(message, "ApplyGammaShift:"))
        return ENGINE_TRACE_CAT_WEATHER;
    if (engine_trace_starts_with(message, "palette_cache:")
     || engine_trace_starts_with(message, "clear_framebuffer:")
     || engine_trace_starts_with(message, "precompute_windows:")
     || engine_trace_starts_with(message, "backdrop_init:")
     || engine_trace_starts_with(message, "render_bg:")
     || engine_trace_starts_with(message, "render_sprites:")
     || engine_trace_starts_with(message, "compose:")
     || engine_trace_starts_with(message, "engine_video_render_frame:"))
        return ENGINE_TRACE_CAT_RENDERER;
    if (engine_trace_starts_with(message, "MainLoop:")
     || engine_trace_starts_with(message, "PlttTransfer:")
     || engine_trace_starts_with(message, "WaitForVBlank:")
     || engine_trace_starts_with(message, "RuntimeVBlank:"))
        return ENGINE_TRACE_CAT_MAINLOOP;
    return ENGINE_TRACE_CAT_GENERAL;
}

static uint32_t engine_runtime_trace_mask_from_token(const char *start, size_t length)
{
    if (engine_trace_token_equals(start, length, "all"))
        return 0xFFFFFFFFu;
    if (engine_trace_token_equals(start, length, "default"))
        return ENGINE_TRACE_DEFAULT_MASK;
    if (engine_trace_token_equals(start, length, "none"))
        return 0u;
    if (engine_trace_token_equals(start, length, "crash"))
        return ENGINE_TRACE_CAT_CRASH;
    if (engine_trace_token_equals(start, length, "battle"))
        return ENGINE_TRACE_CAT_BATTLE;
    if (engine_trace_token_equals(start, length, "task"))
        return ENGINE_TRACE_CAT_TASK;
    if (engine_trace_token_equals(start, length, "move"))
        return ENGINE_TRACE_CAT_MOVE;
    if (engine_trace_token_equals(start, length, "weather"))
        return ENGINE_TRACE_CAT_WEATHER;
    if (engine_trace_token_equals(start, length, "renderer"))
        return ENGINE_TRACE_CAT_RENDERER;
    if (engine_trace_token_equals(start, length, "mainloop"))
        return ENGINE_TRACE_CAT_MAINLOOP;
    if (engine_trace_token_equals(start, length, "warn"))
        return ENGINE_TRACE_CAT_WARN;
    if (engine_trace_token_equals(start, length, "general"))
        return ENGINE_TRACE_CAT_GENERAL;
    return 0u;
}

static void engine_runtime_trace_init_config(void)
{
    const char *env;

    if (s_trace_config_initialized)
        return;

    s_trace_config_initialized = 1;
    env = getenv("NATIVECART_TRACE");
    if (env == NULL || *env == '\0')
    {
        s_trace_mask = 0u;
        return;
    }

    if (strcmp(env, "none") == 0 || strcmp(env, "0") == 0)
    {
        s_trace_mask = 0u;
        return;
    }

    if (strcmp(env, "all") == 0)
    {
        s_trace_mask = 0xFFFFFFFFu;
        return;
    }

    {
        const char *cursor = env;
        uint32_t mask = 0u;

        while (*cursor != '\0')
        {
            const char *token_start;
            size_t token_length;

            while (*cursor == ' ' || *cursor == '\t' || *cursor == ',' || *cursor == ';')
                ++cursor;
            if (*cursor == '\0')
                break;

            token_start = cursor;
            while (*cursor != '\0' && *cursor != ',' && *cursor != ';')
                ++cursor;
            token_length = (size_t)(cursor - token_start);
            while (token_length > 0 && (token_start[token_length - 1] == ' ' || token_start[token_length - 1] == '\t'))
                --token_length;

            if (token_length > 0)
            {
                uint32_t token_mask = engine_runtime_trace_mask_from_token(token_start, token_length);
                if (token_mask == 0xFFFFFFFFu)
                {
                    mask = token_mask;
                    break;
                }
                if (engine_trace_token_equals(token_start, token_length, "default"))
                    mask |= ENGINE_TRACE_DEFAULT_MASK;
                else
                    mask |= token_mask;
            }
        }

        s_trace_mask = mask;
    }
}

static int engine_runtime_any_trace_enabled(void)
{
    engine_runtime_trace_init_config();
    return s_trace_mask != 0u;
}

static int engine_runtime_trace_should_emit(const char *message)
{
    uint32_t category;

    engine_runtime_trace_init_config();
    if (s_trace_mask == 0u)
        return 0;
    category = engine_runtime_trace_category_for_message(message);
    return (s_trace_mask & category) != 0;
}

static uint64_t engine_runtime_trace_now_ms(void)
{
#ifdef _WIN32
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
#endif
}

#ifdef PORTABLE
static void engine_runtime_dump_palette_stage16_once(void)
{
    static int dumped;
    char buffer[256];
    uint16_t *pltt = (uint16_t *)(uintptr_t)ENGINE_PALETTE_ADDR;

    if (dumped || g_runtime.completed_frames != 16)
        return;

    if (!engine_runtime_any_trace_enabled())
        return;

    dumped = 1;

    snprintf(buffer, sizeof(buffer),
             "PALETTE16 UNFADED %04X %04X %04X %04X %04X %04X %04X %04X",
             gPlttBufferUnfaded[0], gPlttBufferUnfaded[1], gPlttBufferUnfaded[2], gPlttBufferUnfaded[3],
             gPlttBufferUnfaded[4], gPlttBufferUnfaded[5], gPlttBufferUnfaded[6], gPlttBufferUnfaded[7]);
    engine_runtime_trace(buffer);

    snprintf(buffer, sizeof(buffer),
             "PALETTE16 FADED %04X %04X %04X %04X %04X %04X %04X %04X",
             gPlttBufferFaded[0], gPlttBufferFaded[1], gPlttBufferFaded[2], gPlttBufferFaded[3],
             gPlttBufferFaded[4], gPlttBufferFaded[5], gPlttBufferFaded[6], gPlttBufferFaded[7]);
    engine_runtime_trace(buffer);

    snprintf(buffer, sizeof(buffer),
             "PALETTE16 PLTT %04X %04X %04X %04X %04X %04X %04X %04X",
             pltt[0], pltt[1], pltt[2], pltt[3], pltt[4], pltt[5], pltt[6], pltt[7]);
    engine_runtime_trace(buffer);
}

static int engine_runtime_should_dump_overworld_frames(unsigned long frame)
{
    return frame == 1900 || frame == 1950 || frame == 2000 || frame == 2050;
}

static void engine_runtime_dump_overworld_state(unsigned long frame)
{
    char buffer[256];
    u16 dispcnt = GetGpuReg(REG_OFFSET_DISPCNT);
    uint8_t *vram = (uint8_t *)(uintptr_t)ENGINE_VRAM_ADDR;
    int bg;
    const char *cb1_name = "other";
    const char *cb2_name = "other";

    if (!engine_runtime_should_dump_overworld_frames(frame))
        return;

    if (!engine_runtime_any_trace_enabled())
        return;

    if (gMain.callback1 == CB1_Overworld)
        cb1_name = "CB1_Overworld";

    if (gMain.callback2 == CB2_NewGame)
        cb2_name = "CB2_NewGame";
    else if (gMain.callback2 == firered_portable_get_cb2_overworld())
        cb2_name = "CB2_Overworld";
    else if (gMain.callback2 == firered_portable_get_cb2_overworld_basic())
        cb2_name = "CB2_OverworldBasic";

    snprintf(buffer, sizeof(buffer),
             "FRAME %lu cb1=%p(%s) cb2=%p(%s) vblank=%p DISPCNT=%04X PALFADE active=%u mp1=%08lX faded=%04X %04X %04X %04X",
             frame,
             gMain.callback1,
             cb1_name,
             gMain.callback2,
             cb2_name,
             gMain.vblankCallback,
             dispcnt,
             gPaletteFade.active,
             (unsigned long)gPaletteFade.multipurpose1,
             gPlttBufferFaded[0],
             gPlttBufferFaded[1],
              gPlttBufferFaded[2],
              gPlttBufferFaded[3]);
    engine_runtime_trace(buffer);

    for (bg = 0; bg < 4; ++bg)
    {
        u16 bgcnt = GetGpuReg(REG_OFFSET_BG0CNT + bg * 2);
        u32 screen_base = ((bgcnt >> 8) & 0x1Fu) * 0x800u;
        u32 char_base = ((bgcnt >> 2) & 0x3u) * 0x4000u;
        int screen_size = (bgcnt >> 14) & 0x3;
        int screen_width_tiles = (screen_size == 1 || screen_size == 3) ? 64 : 32;
        int screen_height_tiles = (screen_size == 2 || screen_size == 3) ? 64 : 32;
        int hofs = GetGpuReg(REG_OFFSET_BG0HOFS + bg * 4) & 0x1FF;
        int vofs = GetGpuReg(REG_OFFSET_BG0VOFS + bg * 4) & 0x1FF;
        int screen_tile_x = (hofs >> 3) & (screen_width_tiles - 1);
        int screen_tile_y = (vofs >> 3) & (screen_height_tiles - 1);
        u32 visible_tilemap_offset = screen_base + (u32)((screen_tile_y * screen_width_tiles + screen_tile_x) * 2);
        u16 tile = 0;
        u16 visible_tile = 0;
        u16 visible_tile_index = 0;
        u32 visible_tile_offset = char_base;

        if (screen_base <= ENGINE_VRAM_SIZE - 2u)
            tile = *(u16 *)(void *)(vram + screen_base);
        if (visible_tilemap_offset <= ENGINE_VRAM_SIZE - 2u)
        {
            visible_tile = *(u16 *)(void *)(vram + visible_tilemap_offset);
            visible_tile_index = visible_tile & 0x03FFu;
            visible_tile_offset = char_base + visible_tile_index * ((bgcnt & 0x0080u) ? 64u : 32u);
        }

        snprintf(buffer, sizeof(buffer),
                 "FRAME %lu BG%dCNT=%04X hofs=%03d vofs=%03d screen=%04lX char=%04lX tile0=%04X vis=%04X visbytes=%02X %02X %02X %02X",
                 frame,
                 bg,
                 bgcnt,
                 hofs,
                 vofs,
                 (unsigned long)screen_base,
                 (unsigned long)char_base,
                 tile,
                 visible_tile,
                 vram[visible_tile_offset + 0],
                 vram[visible_tile_offset + 1],
                 vram[visible_tile_offset + 2],
                 vram[visible_tile_offset + 3]);
        engine_runtime_trace(buffer);
    }
}

static void engine_runtime_trace_bedroom_movement(unsigned long frame)
{
    char buffer[192];

    if (!engine_runtime_any_trace_enabled())
        return;

    if (frame < 7000 || frame > 10000 || (frame % 100ul) != 0)
        return;

    if (frame == 7000)
    {
        snprintf(buffer, sizeof(buffer),
                 "MOVE target map=PalletTown_PlayersHouse_2F stair=(10,2) destGroup=4 destMap=0");
        engine_runtime_trace(buffer);
    }

    if (gSaveBlock1Ptr == NULL)
        return;

    snprintf(buffer, sizeof(buffer),
             "MOVE frame=%lu pos=(%d,%d) map=(%u,%u)",
             frame,
             gSaveBlock1Ptr->pos.x,
             gSaveBlock1Ptr->pos.y,
             gSaveBlock1Ptr->location.mapGroup,
             gSaveBlock1Ptr->location.mapNum);
    engine_runtime_trace(buffer);
}
#endif

static int engine_runtime_should_log_frame(unsigned long frame)
{
    return frame == 16 || (frame % 300ul) == 0;
}

static int engine_runtime_should_log_frame_step(unsigned long frame)
{
    return 0;
}

static void engine_runtime_trace(const char *message)
{
#ifdef NDEBUG
    (void)message;
    return;
#else
    int head;
    int tail;
    int slot;

    if (message == NULL) {
        return;
    }
    engine_runtime_trace_init_config();
    if (s_trace_mask == 0u) {
        return;
    }
    if (!engine_runtime_trace_should_emit(message)) {
        return;
    }

    for (;;) {
        head = (int)engine_trace_atomic_load(&s_trace_head);
        tail = (int)engine_trace_atomic_load(&s_trace_tail);
        if ((unsigned int)(head - tail) >= ENGINE_TRACE_RING_SIZE) {
            engine_trace_atomic_increment(&s_trace_dropped);
            return;
        }
        if (engine_trace_atomic_compare_exchange(&s_trace_head, head, head + 1)) {
            break;
        }
    }

    slot = head % ENGINE_TRACE_RING_SIZE;
    snprintf(s_trace_ring[slot].line, sizeof(s_trace_ring[slot].line), "%s", message);
    engine_trace_atomic_store(&s_trace_ready[slot], 1);
#endif
}

static void engine_runtime_trace_flush(void)
{
#ifdef NDEBUG
    return;
#else
    int tail;
    int head;
    long dropped;

    tail = (int)engine_trace_atomic_load(&s_trace_tail);
    head = (int)engine_trace_atomic_load(&s_trace_head);
    dropped = (long)engine_trace_atomic_exchange(&s_trace_dropped, 0);

    if (tail >= head && dropped == 0) {
        s_trace_last_flush_ms = engine_runtime_trace_now_ms();
        return;
    }

    while (1) {
        tail = (int)engine_trace_atomic_load(&s_trace_tail);
        head = (int)engine_trace_atomic_load(&s_trace_head);
        if (tail >= head) {
            break;
        }
        if (!engine_trace_atomic_load(&s_trace_ready[tail % ENGINE_TRACE_RING_SIZE])) {
            break;
        }

        fputs(s_trace_ring[tail % ENGINE_TRACE_RING_SIZE].line, stdout);
        fputc('\n', stdout);
        engine_trace_atomic_store(&s_trace_ready[tail % ENGINE_TRACE_RING_SIZE], 0);
        engine_trace_atomic_store(&s_trace_tail, tail + 1);
    }

    fflush(stdout);

    if (dropped > 0) {
        printf("engine_runtime_trace: dropped %ld entries\n", dropped);
        fflush(stdout);
    }

    s_trace_last_flush_ms = engine_runtime_trace_now_ms();
#endif
}

void engine_backend_trace_external(const char *message)
{
    engine_runtime_trace(message);
}

unsigned long engine_backend_get_completed_frame_external(void)
{
    return g_runtime.completed_frames;
}

void engine_backend_trace_bytes_external(const char *label, const void *src, const void *dst)
{
    char buffer[256];

#ifdef NDEBUG
    (void)label;
    (void)src;
    (void)dst;
    return;
#endif

    if (src == NULL)
    {
        snprintf(buffer, sizeof(buffer), "%s src=NULL dst=%p", label, dst);
        engine_runtime_trace(buffer);
        return;
    }

    {
        const unsigned char *bytes = src;
        snprintf(buffer, sizeof(buffer),
                 "%s src=%p bytes=%02X %02X %02X %02X %02X %02X %02X %02X dst=%p",
                 label,
                 src,
                 bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
                 dst);
    }
    engine_runtime_trace(buffer);
}

static void engine_lock(void) {
#ifdef _WIN32
    EnterCriticalSection(&g_runtime.mutex);
#else
    pthread_mutex_lock(&g_runtime.mutex);
#endif
}

static void engine_unlock(void) {
#ifdef _WIN32
    LeaveCriticalSection(&g_runtime.mutex);
#else
    pthread_mutex_unlock(&g_runtime.mutex);
#endif
}

static void engine_signal_all(void) {
#ifdef _WIN32
    WakeAllConditionVariable(&g_runtime.cond);
#else
    pthread_cond_broadcast(&g_runtime.cond);
#endif
}

static void engine_wait(void) {
#ifdef _WIN32
    SleepConditionVariableCS(&g_runtime.cond, &g_runtime.mutex, INFINITE);
#else
    pthread_cond_wait(&g_runtime.cond, &g_runtime.mutex);
#endif
}

static int engine_runtime_begin_idle_mutation(void)
{
    if (!g_runtime.initialized) {
        return 0;
    }

    engine_lock();
    if (g_runtime.shutdown_requested || g_runtime.requested_frames != g_runtime.completed_frames) {
        engine_unlock();
        return 0;
    }

    return 1;
}

static void engine_runtime_end_idle_mutation(void)
{
    engine_unlock();
}

static void engine_runtime_free_rom(void) {
    free(g_runtime.rom_copy);
    g_runtime.rom_copy = NULL;
    g_runtime.rom_size = 0;
}

static int engine_runtime_copy_rom(const uint8_t *rom, size_t rom_size) {
    engine_runtime_free_rom();
    g_runtime.rom_copy = (uint8_t *)malloc(rom_size);
    if (g_runtime.rom_copy == NULL) {
        return 0;
    }
    memcpy(g_runtime.rom_copy, rom, rom_size);
    g_runtime.rom_size = rom_size;
    return 1;
}

#ifdef _WIN32
static DWORD WINAPI engine_engine_thread(void *unused) {
    (void)unused;
    engine_runtime_trace("AgbMain: entering");
    AgbMain();
    engine_runtime_trace("AgbMain: exited");
    return 0;
}
#else
static void *engine_engine_thread(void *unused) {
    (void)unused;
    engine_runtime_trace("AgbMain: entering");
    AgbMain();
    engine_runtime_trace("AgbMain: exited");
    return NULL;
}
#endif

static int engine_start_engine_thread(void) {
#ifdef _WIN32
    g_runtime.thread = CreateThread(NULL, 0, engine_engine_thread, NULL, 0, NULL);
    return g_runtime.thread != NULL;
#else
    return pthread_create(&g_runtime.thread, NULL, engine_engine_thread, NULL) == 0;
#endif
}

static void engine_join_engine_thread(void) {
#ifdef _WIN32
    if (g_runtime.thread != NULL) {
        WaitForSingleObject(g_runtime.thread, INFINITE);
        CloseHandle(g_runtime.thread);
        g_runtime.thread = NULL;
    }
#else
    if (g_runtime.thread) {
        pthread_join(g_runtime.thread, NULL);
        memset(&g_runtime.thread, 0, sizeof(g_runtime.thread));
    }
#endif
}

int engine_backend_init(const uint8_t *rom, size_t rom_size) {
    engine_runtime_trace("engine_backend_init: enter");
    memset(&g_runtime, 0, sizeof(g_runtime));

#ifdef _WIN32
    InitializeCriticalSection(&g_runtime.mutex);
    InitializeConditionVariable(&g_runtime.cond);
#else
    pthread_mutex_init(&g_runtime.mutex, NULL);
    pthread_cond_init(&g_runtime.cond, NULL);
#endif

    if (rom == NULL || rom_size == 0) {
        engine_runtime_trace("engine_backend_init: invalid rom");
        engine_backend_shutdown();
        return 0;
    }

    if (!engine_runtime_copy_rom(rom, rom_size)) {
        engine_runtime_trace("engine_backend_init: rom copy failed");
        engine_backend_shutdown();
        return 0;
    }

    if (!engine_memory_init(g_runtime.rom_copy, g_runtime.rom_size)) {
        engine_runtime_trace("engine_backend_init: memory init failed");
        engine_backend_shutdown();
        return 0;
    }

#ifdef PORTABLE
    firered_portable_init_map_object_event_script_words();
#endif

    engine_backend_input_reset();
    engine_audio_reset();
    engine_video_reset();

    g_runtime.initialized = 1;
    g_runtime.shutdown_requested = 0;
    g_runtime.soft_reset_requested = 0;
    g_runtime.requested_frames = 1;
    g_runtime.completed_frames = 0;

    if (!engine_start_engine_thread()) {
        engine_runtime_trace("engine_backend_init: engine thread start failed");
        engine_backend_shutdown();
        return 0;
    }

    engine_runtime_trace("engine_backend_init: success");
    return 1;
}

void engine_backend_reset(void) {
    uint8_t *rom_copy = NULL;
    size_t rom_size = g_runtime.rom_size;

    if (g_runtime.rom_copy != NULL && rom_size != 0) {
        rom_copy = (uint8_t *)malloc(rom_size);
        if (rom_copy != NULL) {
            memcpy(rom_copy, g_runtime.rom_copy, rom_size);
        }
    }

    engine_backend_shutdown();

    if (rom_copy != NULL) {
        engine_backend_init(rom_copy, rom_size);
        free(rom_copy);
    }
}

void engine_backend_set_input(uint16_t buttons) {
    engine_backend_input_set_buttons(buttons);
}

void engine_backend_run_frame(void) {
    unsigned long target_frame;
    unsigned long completed_frame;
    uint64_t now_ms;
    char buffer[64];

    if (!g_runtime.initialized) {
        return;
    }

    engine_lock();
    g_runtime.requested_frames += 1;
    target_frame = g_runtime.requested_frames;
    engine_signal_all();

    while (!g_runtime.shutdown_requested && g_runtime.completed_frames < target_frame) {
        engine_wait();
    }
    completed_frame = g_runtime.completed_frames;
    engine_unlock();

    if (engine_runtime_any_trace_enabled() && engine_runtime_should_log_frame_step(completed_frame)) {
        snprintf(buffer, sizeof(buffer), "engine_backend_run_frame: frame=%lu", completed_frame);
        engine_runtime_trace(buffer);
    }

    if (!g_runtime.shutdown_requested) {
#ifdef PORTABLE
        if (gMain.vblankCallback != NULL && gMain.callback2 != CB2_InitBattle)
            gMain.vblankCallback();
#else
        if (gMain.vblankCallback != NULL)
            gMain.vblankCallback();
#endif
        CopyBufferedValuesToGpuRegs();
        ProcessDma3Requests();
#ifdef PORTABLE
        engine_runtime_dump_palette_stage16_once();
        engine_runtime_dump_overworld_state(completed_frame);
        engine_runtime_trace_bedroom_movement(completed_frame);
        if (engine_runtime_any_trace_enabled()
            && completed_frame >= 7600 && completed_frame <= 7900) {
            snprintf(buffer, sizeof(buffer), "engine_backend_run_frame: pre-video frame=%lu", completed_frame);
            engine_runtime_trace(buffer);
        }
#endif
        if (engine_runtime_any_trace_enabled() && engine_runtime_should_log_frame(completed_frame)) {
            snprintf(buffer, sizeof(buffer), "engine_backend_run_frame: frame=%lu", completed_frame);
            engine_runtime_trace(buffer);
        }
        engine_video_render_frame();
#ifdef PORTABLE
        if (engine_runtime_any_trace_enabled()
            && completed_frame >= 7600 && completed_frame <= 7900) {
            snprintf(buffer, sizeof(buffer), "engine_backend_run_frame: post-video frame=%lu", completed_frame);
            engine_runtime_trace(buffer);
        }
#endif
        now_ms = engine_runtime_trace_now_ms();
        if (engine_runtime_any_trace_enabled()
            && now_ms >= s_trace_last_flush_ms + 1000ull) {
            engine_runtime_trace_flush();
        }
    }

    if (g_runtime.soft_reset_requested) {
        g_runtime.soft_reset_requested = 0;
        engine_backend_reset();
    }
}

int engine_backend_open_start_menu(void)
{
#ifdef PORTABLE
    int success = 0;

    if (!engine_runtime_begin_idle_mutation()) {
        return 0;
    }

    if (gMain.callback2 == firered_portable_get_cb2_overworld()
     || gMain.callback2 == firered_portable_get_cb2_overworld_basic()) {
        CB2_ReturnToFieldWithOpenMenu();
        success = 1;
    }

    engine_runtime_end_idle_mutation();
    return success;
#else
    return 0;
#endif
}

int engine_backend_save_state(const char *path)
{
    EngineStateFileHeader header = {{'F', 'R', 'S', 'T', 'A', 'T', 'E', '1'}, 0};
    FILE *out;
    int success = 0;

    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    if (!engine_runtime_begin_idle_mutation()) {
        return 0;
    }

    header.version = ENGINE_STATE_FILE_VERSION;
    header.ewram_size = ENGINE_EWRAM_SIZE;
    header.iwram_size = ENGINE_IWRAM_SIZE;
    header.ioreg_size = ENGINE_IOREG_SIZE;
    header.palette_size = ENGINE_PALETTE_SIZE;
    header.vram_size = ENGINE_VRAM_SIZE;
    header.oam_size = ENGINE_OAM_SIZE;
    header.sram_size = ENGINE_SRAM_SIZE;

    out = fopen(path, "wb");
    if (out != NULL
     && fwrite(&header, sizeof(header), 1, out) == 1
     && fwrite((const void *)(uintptr_t)ENGINE_EWRAM_ADDR, ENGINE_EWRAM_SIZE, 1, out) == 1
     && fwrite((const void *)(uintptr_t)ENGINE_IWRAM_ADDR, ENGINE_IWRAM_SIZE, 1, out) == 1
     && fwrite((const void *)(uintptr_t)ENGINE_IOREG_ADDR, ENGINE_IOREG_SIZE, 1, out) == 1
     && fwrite((const void *)(uintptr_t)ENGINE_PALETTE_ADDR, ENGINE_PALETTE_SIZE, 1, out) == 1
     && fwrite((const void *)(uintptr_t)ENGINE_VRAM_ADDR, ENGINE_VRAM_SIZE, 1, out) == 1
     && fwrite((const void *)(uintptr_t)ENGINE_OAM_ADDR, ENGINE_OAM_SIZE, 1, out) == 1
     && fwrite((const void *)(uintptr_t)ENGINE_SRAM_ADDR, ENGINE_SRAM_SIZE, 1, out) == 1) {
        success = 1;
    }

    if (out != NULL) {
        fclose(out);
    }

    engine_runtime_end_idle_mutation();
    return success;
}

int engine_backend_load_state(const char *path)
{
    EngineStateFileHeader header;
    FILE *in;
    int success = 0;

    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    if (!engine_runtime_begin_idle_mutation()) {
        return 0;
    }

    in = fopen(path, "rb");
    if (in != NULL
     && fread(&header, sizeof(header), 1, in) == 1
     && memcmp(header.magic, "FRSTATE1", sizeof(header.magic)) == 0
     && header.version == ENGINE_STATE_FILE_VERSION
     && header.ewram_size == ENGINE_EWRAM_SIZE
     && header.iwram_size == ENGINE_IWRAM_SIZE
     && header.ioreg_size == ENGINE_IOREG_SIZE
     && header.palette_size == ENGINE_PALETTE_SIZE
     && header.vram_size == ENGINE_VRAM_SIZE
     && header.oam_size == ENGINE_OAM_SIZE
     && header.sram_size == ENGINE_SRAM_SIZE
     && fread((void *)(uintptr_t)ENGINE_EWRAM_ADDR, ENGINE_EWRAM_SIZE, 1, in) == 1
     && fread((void *)(uintptr_t)ENGINE_IWRAM_ADDR, ENGINE_IWRAM_SIZE, 1, in) == 1
     && fread((void *)(uintptr_t)ENGINE_IOREG_ADDR, ENGINE_IOREG_SIZE, 1, in) == 1
     && fread((void *)(uintptr_t)ENGINE_PALETTE_ADDR, ENGINE_PALETTE_SIZE, 1, in) == 1
     && fread((void *)(uintptr_t)ENGINE_VRAM_ADDR, ENGINE_VRAM_SIZE, 1, in) == 1
     && fread((void *)(uintptr_t)ENGINE_OAM_ADDR, ENGINE_OAM_SIZE, 1, in) == 1
     && fread((void *)(uintptr_t)ENGINE_SRAM_ADDR, ENGINE_SRAM_SIZE, 1, in) == 1) {
        CopyBufferedValuesToGpuRegs();
        ProcessDma3Requests();
        engine_video_render_frame();
        success = 1;
    }

    if (in != NULL) {
        fclose(in);
    }

    engine_runtime_end_idle_mutation();
    return success;
}

const uint8_t *engine_backend_get_framebuffer_rgba(void) {
    return engine_video_get_framebuffer();
}

int engine_backend_get_frame_width(void) {
    return ENGINE_GBA_WIDTH;
}

int engine_backend_get_frame_height(void) {
    return ENGINE_GBA_HEIGHT;
}

int16_t *engine_backend_get_audio_samples(size_t *count) {
    return engine_audio_get_samples(count);
}

void engine_backend_shutdown(void) {
    if (g_runtime.initialized) {
        engine_lock();
        g_runtime.shutdown_requested = 1;
        engine_signal_all();
        engine_unlock();
        engine_join_engine_thread();

#ifdef _WIN32
        DeleteCriticalSection(&g_runtime.mutex);
#else
        pthread_cond_destroy(&g_runtime.cond);
        pthread_mutex_destroy(&g_runtime.mutex);
#endif
    }

    engine_runtime_trace_flush();

    engine_memory_shutdown();
    engine_runtime_free_rom();

    memset(&g_runtime, 0, sizeof(g_runtime));
}

void engine_backend_vblank_wait(void) {
    engine_lock();
#ifdef PORTABLE
    if (engine_runtime_any_trace_enabled() && gMain.callback2 != NULL) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "RuntimeVBlank: enter req=%lu done=%lu cb2=%p vblank=%p",
                 g_runtime.requested_frames, g_runtime.completed_frames, gMain.callback2, gMain.vblankCallback);
        engine_runtime_trace(buffer);
    }
#endif
    g_runtime.completed_frames += 1;
    engine_signal_all();

#ifdef PORTABLE
    if (engine_runtime_any_trace_enabled() && gMain.callback2 != NULL) {
        char buffer[96];
        snprintf(buffer, sizeof(buffer), "RuntimeVBlank: signaled req=%lu done=%lu",
                 g_runtime.requested_frames, g_runtime.completed_frames);
        engine_runtime_trace(buffer);
    }
#endif

    while (!g_runtime.shutdown_requested && g_runtime.requested_frames <= g_runtime.completed_frames) {
#ifdef PORTABLE
        if (engine_runtime_any_trace_enabled() && gMain.callback2 != NULL)
            engine_runtime_trace("RuntimeVBlank: waiting for next frame request");
#endif
        engine_wait();
    }

    if (g_runtime.shutdown_requested) {
        engine_unlock();
#ifdef _WIN32
        ExitThread(0);
#else
        pthread_exit(NULL);
#endif
    }

    engine_unlock();
#ifdef PORTABLE
    if (engine_runtime_any_trace_enabled() && gMain.callback2 != NULL)
        engine_runtime_trace("RuntimeVBlank: exit");
#endif
}

void engine_backend_request_soft_reset(void) {
    g_runtime.soft_reset_requested = 1;
}
