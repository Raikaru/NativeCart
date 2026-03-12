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

static uint16_t sdl_shell_collect_input(void)
{
    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    uint16_t buttons = 0;

    if (keys[SDL_SCANCODE_X]) buttons |= GBA_BUTTON_A;
    if (keys[SDL_SCANCODE_Z]) buttons |= GBA_BUTTON_B;
    if (keys[SDL_SCANCODE_BACKSPACE]) buttons |= GBA_BUTTON_SELECT;
    if (keys[SDL_SCANCODE_RETURN]) buttons |= GBA_BUTTON_START;
    if (keys[SDL_SCANCODE_RIGHT]) buttons |= GBA_BUTTON_RIGHT;
    if (keys[SDL_SCANCODE_LEFT]) buttons |= GBA_BUTTON_LEFT;
    if (keys[SDL_SCANCODE_UP]) buttons |= GBA_BUTTON_UP;
    if (keys[SDL_SCANCODE_DOWN]) buttons |= GBA_BUTTON_DOWN;
    if (keys[SDL_SCANCODE_S]) buttons |= GBA_BUTTON_R;
    if (keys[SDL_SCANCODE_A]) buttons |= GBA_BUTTON_L;

    return buttons;
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

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ctx->renderer == NULL)
    {
        ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED);
    }
    if (ctx->renderer == NULL)
    {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_RenderSetLogicalSize(ctx->renderer, frame.width, frame.height);
    ctx->texture = SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, frame.width, frame.height);
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

    if (frame.rgba == NULL || frame.width <= 0 || frame.height <= 0)
        return false;

    if (SDL_UpdateTexture(ctx->texture, NULL, frame.rgba, frame.width * 4) != 0)
    {
        fprintf(stderr, "SDL_UpdateTexture failed: %s\n", SDL_GetError());
        return false;
    }

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

    if (argc < 2)
    {
        sdl_shell_print_usage(argv[0]);
        return 1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.scale = SDL_SHELL_DEFAULT_SCALE;
    ctx.state_path = (argc >= 3) ? argv[2] : SDL_SHELL_DEFAULT_STATE_PATH;
    ctx.running = true;

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

        while (SDL_PollEvent(&event))
            sdl_shell_handle_event(&ctx, &event);

        engine_input_set_buttons(sdl_shell_collect_input());
        engine_run_frame();

        if (!sdl_shell_update_video(&ctx))
            break;
        sdl_shell_update_audio(&ctx);
    }

    sdl_shell_shutdown(&ctx);
    return 0;
}
