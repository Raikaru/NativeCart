#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../core/core_loader.h"
#include "../../core/engine_audio.h"
#include "../../core/engine_input.h"
#include "../../core/engine_runtime.h"
#include "../../core/engine_video.h"
#include "../../../cores/firered/firered_core.h"

#define SDL_SHELL_DEFAULT_SCALE 4
#define SDL_SHELL_DEFAULT_STATE_PATH "sdl_shell.state"
#define SDL_SHELL_AUDIO_FREQUENCY 32768
#define SDL_SHELL_AUDIO_CHANNELS 2
#define SDL_SHELL_MAX_AUDIO_QUEUE_BYTES (SDL_SHELL_AUDIO_FREQUENCY * SDL_SHELL_AUDIO_CHANNELS * (int)sizeof(int16_t))
#define SDL_SHELL_RENDERER_FLAGS SDL_RENDERER_ACCELERATED
#define SDL_SHELL_TARGET_FPS 60.0

enum
{
    GBA_BUTTON_A      = 0x0001,
    GBA_BUTTON_B      = 0x0002,
    GBA_BUTTON_SELECT = 0x0004,
    GBA_BUTTON_START  = 0x0008,
    GBA_BUTTON_RIGHT  = 0x0010,
    GBA_BUTTON_LEFT   = 0x0020,
    GBA_BUTTON_UP     = 0x0040,
    GBA_BUTTON_DOWN   = 0x0080,
    GBA_BUTTON_R      = 0x0100,
    GBA_BUTTON_L      = 0x0200,
};

typedef struct SdlShellContext
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_AudioDeviceID audio_device;
    int scale;
    const char *state_path;
    uint16_t input_state;
    uint16_t input_latched;
    bool has_focus;
    bool running;
} SdlShellContext;

static void sdl_shell_print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s <rom.gba> [state_file]\n", program);
}

static uint8_t *sdl_shell_read_file(const char *path, size_t *size_out)
{
    FILE *file;
    long file_size;
    uint8_t *buffer;

    *size_out = 0;
    file = fopen(path, "rb");
    if (file == NULL)
        return NULL;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }

    file_size = ftell(file);
    if (file_size <= 0)
    {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }

    buffer = malloc((size_t)file_size);
    if (buffer == NULL)
    {
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, (size_t)file_size, file) != (size_t)file_size)
    {
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);
    *size_out = (size_t)file_size;
    return buffer;
}

static uint16_t sdl_shell_key_to_gba_button(SDL_Scancode scancode)
{
    switch (scancode)
    {
    case SDL_SCANCODE_Z:
        return GBA_BUTTON_A;
    case SDL_SCANCODE_X:
        return GBA_BUTTON_B;
    case SDL_SCANCODE_BACKSPACE:
        return GBA_BUTTON_SELECT;
    case SDL_SCANCODE_RETURN:
        return GBA_BUTTON_START;
    case SDL_SCANCODE_RIGHT:
        return GBA_BUTTON_RIGHT;
    case SDL_SCANCODE_LEFT:
        return GBA_BUTTON_LEFT;
    case SDL_SCANCODE_UP:
        return GBA_BUTTON_UP;
    case SDL_SCANCODE_DOWN:
        return GBA_BUTTON_DOWN;
    case SDL_SCANCODE_S:
        return GBA_BUTTON_R;
    case SDL_SCANCODE_A:
        return GBA_BUTTON_L;
    default:
        return 0;
    }
}

static uint16_t sdl_shell_collect_input(const SdlShellContext *ctx)
{
    if (!ctx->has_focus)
        return 0;

    return ctx->input_state | ctx->input_latched;
}

static bool sdl_shell_init_audio(SdlShellContext *ctx)
{
    SDL_AudioSpec desired;

    SDL_zero(desired);
    desired.freq = SDL_SHELL_AUDIO_FREQUENCY;
    desired.format = AUDIO_S16SYS;
    desired.channels = SDL_SHELL_AUDIO_CHANNELS;
    desired.samples = 1024;
    desired.callback = NULL;

    ctx->audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
    if (ctx->audio_device == 0)
    {
        fprintf(stderr, "SDL audio disabled: %s\n", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(ctx->audio_device, 0);
    return true;
}

static bool sdl_shell_init_video(SdlShellContext *ctx)
{
    EngineVideoFrame frame = engine_video_get_frame();

    if (frame.rgba == NULL || frame.width <= 0 || frame.height <= 0)
    {
        fprintf(stderr, "Engine framebuffer is unavailable\n");
        return false;
    }

    ctx->window = SDL_CreateWindow(
        "decomp-engine SDL shell",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        frame.width * ctx->scale,
        frame.height * ctx->scale,
        SDL_WINDOW_SHOWN);
    if (ctx->window == NULL)
    {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_SHELL_RENDERER_FLAGS);
    if (ctx->renderer == NULL)
    {
        ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (ctx->renderer == NULL)
    {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_RenderSetLogicalSize(ctx->renderer, frame.width, frame.height);
    ctx->texture = SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, frame.width, frame.height);
    if (ctx->texture == NULL)
    {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

static void sdl_shell_update_audio(SdlShellContext *ctx)
{
    EngineAudioBuffer buffer;

    if (ctx->audio_device == 0)
        return;

    buffer = engine_audio_get_buffer();
    if (buffer.samples == NULL || buffer.sample_count == 0)
        return;

    if ((int)SDL_GetQueuedAudioSize(ctx->audio_device) > SDL_SHELL_MAX_AUDIO_QUEUE_BYTES)
        SDL_ClearQueuedAudio(ctx->audio_device);

    SDL_QueueAudio(ctx->audio_device, buffer.samples, (uint32_t)(buffer.sample_count * sizeof(int16_t)));
}

static bool sdl_shell_update_video(SdlShellContext *ctx)
{
    EngineVideoFrame frame = engine_video_get_frame();
    void *pixels;
    int pitch;
    int y;

    if (frame.rgba == NULL || frame.width <= 0 || frame.height <= 0)
        return false;

    if (SDL_LockTexture(ctx->texture, NULL, &pixels, &pitch) != 0)
    {
        fprintf(stderr, "SDL_LockTexture failed: %s\n", SDL_GetError());
        return false;
    }

    if (pitch == frame.width * 4)
    {
        memcpy(pixels, frame.rgba, (size_t)(frame.width * frame.height * 4));
    }
    else
    {
        const uint8_t *src = frame.rgba;
        uint8_t *dst = pixels;

        for (y = 0; y < frame.height; y++)
        {
            memcpy(dst, src, (size_t)(frame.width * 4));
            src += frame.width * 4;
            dst += pitch;
        }
    }

    SDL_UnlockTexture(ctx->texture);

    SDL_RenderClear(ctx->renderer);
    SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
    SDL_RenderPresent(ctx->renderer);
    return true;
}

static void sdl_shell_handle_event(SdlShellContext *ctx, const SDL_Event *event)
{
    if (event->type == SDL_QUIT)
    {
        ctx->running = false;
        return;
    }

    if (event->type == SDL_WINDOWEVENT)
    {
        if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
            ctx->has_focus = true;
        else if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            ctx->has_focus = false;
        return;
    }

    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
    {
        uint16_t button = sdl_shell_key_to_gba_button(event->key.keysym.scancode);

        if (button != 0)
        {
            if (event->type == SDL_KEYDOWN)
            {
                ctx->input_state |= button;
                ctx->input_latched |= button;
            }
            else
                ctx->input_state &= (uint16_t)~button;
        }
    }

    if (event->type != SDL_KEYDOWN || event->key.repeat != 0)
        return;

    switch (event->key.keysym.sym)
    {
    case SDLK_ESCAPE:
        ctx->running = false;
        break;
    case SDLK_F1:
        engine_open_start_menu();
        break;
    case SDLK_F5:
        engine_save_state(ctx->state_path);
        break;
    case SDLK_F9:
        if (engine_load_state(ctx->state_path) != 0)
            sdl_shell_update_video(ctx);
        break;
    default:
        break;
    }
}

static void sdl_shell_shutdown(SdlShellContext *ctx)
{
    if (ctx->audio_device != 0)
        SDL_CloseAudioDevice(ctx->audio_device);
    if (ctx->texture != NULL)
        SDL_DestroyTexture(ctx->texture);
    if (ctx->renderer != NULL)
        SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window != NULL)
        SDL_DestroyWindow(ctx->window);
    engine_shutdown();
    SDL_Quit();
}

int main(int argc, char **argv)
{
    SdlShellContext ctx;
    uint8_t *rom_data;
    size_t rom_size;
    Uint64 perf_freq;
    double target_frame_ms;

    if (argc < 2)
    {
        sdl_shell_print_usage(argv[0]);
        return 1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.scale = SDL_SHELL_DEFAULT_SCALE;
    ctx.state_path = (argc >= 3) ? argv[2] : SDL_SHELL_DEFAULT_STATE_PATH;
    ctx.has_focus = true;
    ctx.running = true;
    perf_freq = SDL_GetPerformanceFrequency();
    target_frame_ms = 1000.0 / SDL_SHELL_TARGET_FPS;

    rom_data = sdl_shell_read_file(argv[1], &rom_size);
    if (rom_data == NULL)
    {
        fprintf(stderr, "Failed to read ROM: %s\n", argv[1]);
        return 1;
    }

    engine_set_core(firered_core_get());
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        free(rom_data);
        return 1;
    }

    if (engine_load_rom(rom_data, rom_size) == 0)
    {
        fprintf(stderr, "engine_load_rom failed\n");
        free(rom_data);
        SDL_Quit();
        return 1;
    }
    free(rom_data);

    if (!sdl_shell_init_video(&ctx))
    {
        sdl_shell_shutdown(&ctx);
        return 1;
    }
    sdl_shell_init_audio(&ctx);

    while (ctx.running)
    {
        SDL_Event event;
        Uint64 frame_start;
        Uint64 frame_end;
        double elapsed_ms;

        frame_start = SDL_GetPerformanceCounter();

        while (SDL_PollEvent(&event))
            sdl_shell_handle_event(&ctx, &event);

        engine_input_set_buttons(sdl_shell_collect_input(&ctx));
        engine_run_frame();
        ctx.input_latched = 0;

        if (!sdl_shell_update_video(&ctx))
            break;
        sdl_shell_update_audio(&ctx);

        frame_end = SDL_GetPerformanceCounter();
        elapsed_ms = (double)(frame_end - frame_start) * 1000.0 / (double)perf_freq;
        if (elapsed_ms < target_frame_ms)
            SDL_Delay((Uint32)(target_frame_ms - elapsed_ms));
    }

    sdl_shell_shutdown(&ctx);
    return 0;
}
