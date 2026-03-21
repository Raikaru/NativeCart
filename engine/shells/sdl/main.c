#define SDL_MAIN_HANDLED

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../include/constants/songs.h"
#include "../../../include/m4a.h"
#include "../../core/core_loader.h"
#include "../../core/engine_audio.h"
#include "../../core/engine_input.h"
#include "../../core/engine_runtime.h"
#include "../../core/engine_video.h"
#include "../../core/mod_patch_bps.h"
#include "../../../cores/firered/firered_core.h"
#include "../../../include/gba/flash_internal.h"

#define SDL_SHELL_DEFAULT_SCALE 4
#define SDL_SHELL_DEFAULT_STATE_PATH "sdl_shell.state"
#define SDL_SHELL_AUDIO_FREQUENCY 32768
#define SDL_SHELL_AUDIO_CHANNELS 2
#define SDL_SHELL_MAX_AUDIO_QUEUE_BYTES (SDL_SHELL_AUDIO_FREQUENCY * SDL_SHELL_AUDIO_CHANNELS * (int)sizeof(int16_t))
#define SDL_SHELL_TARGET_FPS 60.0
#define SDL_SHELL_WINDOW_TITLE "decomp-engine SDL shell"

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
    SDL_AudioStream *audio_stream;
    SDL_Gamepad *gamepad;
    int scale;
    const char *state_path;
    uint16_t input_state;
    uint16_t input_latched;
    bool has_focus;
    bool fast_forward_active;
    bool fast_forward_toggled;
    bool audio_selftest_enabled;
    bool audio_activity_marked;
    bool running;
    unsigned int frame_count;
} SdlShellContext;

static void sdl_shell_print_usage(const char *program)
{
    fprintf(
        stderr,
        "Usage: %s <rom.gba> [--bps <patch.bps>] [--mod-manifest <file>] [state_file]\n"
        "       FIRERED_BPS_PATCH may set a default .bps if --bps is not used.\n"
        "       --mod-manifest: first non-empty, non-# line is a .bps path relative to the manifest directory.\n",
        program);
}

static const char *sdl_shell_path_sep_last(const char *path)
{
    const char *slash;
    const char *bslash;

    slash = strrchr(path, '/');
    bslash = strrchr(path, '\\');
    if (slash == NULL)
        return bslash;
    if (bslash == NULL)
        return slash;
    return slash > bslash ? slash : bslash;
}

/*
 * Reads the first non-empty, non-comment line from a tiny manifest file.
 * The line is a path to a .bps relative to the manifest's directory.
 * Returns 1 and writes NUL-terminated absolute/resolved path into out_path if found.
 */
static int sdl_shell_read_mod_manifest_bps(const char *manifest_path, char *out_path, size_t out_cap)
{
    FILE *file;
    char line[768];
    const char *sep;
    size_t dir_len;

    if (manifest_path == NULL || out_path == NULL || out_cap == 0)
        return 0;

    file = fopen(manifest_path, "rb");
    if (file == NULL)
        return 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *start = line;
        char *end;
        size_t len;

        while (*start == ' ' || *start == '\t' || *start == '\r')
            start++;
        if (*start == '\0' || *start == '\n')
            continue;
        if (*start == '#')
            continue;

        end = start + strlen(start);
        while (end > start && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t'))
            end--;
        *end = '\0';

        sep = sdl_shell_path_sep_last(manifest_path);
        if (sep != NULL && sep != manifest_path)
        {
            dir_len = (size_t)(sep - manifest_path);
            if (dir_len + 1 + strlen(start) + 1 > out_cap)
            {
                fclose(file);
                return 0;
            }
            memcpy(out_path, manifest_path, dir_len);
            out_path[dir_len] = '/';
            memcpy(out_path + dir_len + 1, start, strlen(start) + 1);
        }
        else
        {
            if (strlen(start) + 1 > out_cap)
            {
                fclose(file);
                return 0;
            }
            memcpy(out_path, start, strlen(start) + 1);
        }

        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}

typedef struct SdlShellArgs
{
    const char *rom_path;
    const char *state_path;
    const char *bps_path;
    int used_manifest;
    char manifest_bps_storage[1024];
} SdlShellArgs;

static int sdl_shell_parse_args(int argc, char **argv, SdlShellArgs *out)
{
    int i;

    memset(out, 0, sizeof(*out));
    out->state_path = SDL_SHELL_DEFAULT_STATE_PATH;

    if (argc < 2)
        return 0;

    out->rom_path = argv[1];
    i = 2;
    while (i < argc)
    {
        if (strcmp(argv[i], "--bps") == 0)
        {
            if (i + 1 >= argc)
                return 0;
            out->bps_path = argv[i + 1];
            i += 2;
            continue;
        }
        if (strcmp(argv[i], "--mod-manifest") == 0)
        {
            if (i + 1 >= argc)
                return 0;
            if (!sdl_shell_read_mod_manifest_bps(argv[i + 1], out->manifest_bps_storage, sizeof(out->manifest_bps_storage)))
            {
                fprintf(stderr, "Invalid or empty mod manifest: %s\n", argv[i + 1]);
                return 0;
            }
            out->used_manifest = 1;
            i += 2;
            continue;
        }
        if (argv[i][0] == '-')
            return 0;

        out->state_path = argv[i];
        i += 1;
    }

    if (out->bps_path == NULL && out->used_manifest)
        out->bps_path = out->manifest_bps_storage;

    if (out->bps_path == NULL)
    {
        const char *env_bps = getenv("FIRERED_BPS_PATCH");

        if (env_bps != NULL && env_bps[0] != '\0')
            out->bps_path = env_bps;
    }

    return 1;
}

static void sdl_shell_write_marker(const char *path, const char *contents)
{
    FILE *file;

    if (path == NULL || contents == NULL)
        return;

    file = fopen(path, "wb");
    if (file == NULL)
        return;

    fputs(contents, file);
    fclose(file);
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

static uint16_t sdl_shell_gamepad_button_to_gba_button(SDL_GamepadButton button)
{
    switch (button)
    {
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return GBA_BUTTON_A;
    case SDL_GAMEPAD_BUTTON_EAST:
        return GBA_BUTTON_B;
    case SDL_GAMEPAD_BUTTON_BACK:
        return GBA_BUTTON_SELECT;
    case SDL_GAMEPAD_BUTTON_START:
        return GBA_BUTTON_START;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return GBA_BUTTON_RIGHT;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return GBA_BUTTON_LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return GBA_BUTTON_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return GBA_BUTTON_DOWN;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return GBA_BUTTON_R;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return GBA_BUTTON_L;
    default:
        return 0;
    }
}

static void sdl_shell_close_gamepad(SdlShellContext *ctx)
{
    if (ctx->gamepad != NULL)
    {
        SDL_CloseGamepad(ctx->gamepad);
        ctx->gamepad = NULL;
    }
}

static bool sdl_shell_open_gamepad(SdlShellContext *ctx, SDL_JoystickID instance_id)
{
    SDL_Gamepad *gamepad = SDL_OpenGamepad(instance_id);

    if (gamepad == NULL)
    {
        fprintf(stderr, "SDL gamepad open failed: %s\n", SDL_GetError());
        return false;
    }

    sdl_shell_close_gamepad(ctx);
    ctx->gamepad = gamepad;
    fprintf(stderr, "SDL gamepad connected: %s\n", SDL_GetGamepadName(gamepad));
    return true;
}

static void sdl_shell_open_first_gamepad(SdlShellContext *ctx)
{
    int count = 0;
    SDL_JoystickID *gamepads = SDL_GetGamepads(&count);

    if (gamepads == NULL)
        return;

    if (count > 0)
        sdl_shell_open_gamepad(ctx, gamepads[0]);

    SDL_free(gamepads);
}

static uint16_t sdl_shell_collect_input(const SdlShellContext *ctx)
{
    if (!ctx->has_focus)
        return 0;

    return ctx->input_state | ctx->input_latched;
}

static bool sdl_shell_is_fast_forward_active(const SdlShellContext *ctx)
{
    return ctx->fast_forward_active || ctx->fast_forward_toggled;
}

static bool sdl_shell_init_audio(SdlShellContext *ctx)
{
    SDL_AudioSpec desired;

    SDL_zero(desired);
    desired.format = SDL_AUDIO_S16;
    desired.channels = SDL_SHELL_AUDIO_CHANNELS;
    desired.freq = SDL_SHELL_AUDIO_FREQUENCY;

    ctx->audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, NULL, NULL);
    if (ctx->audio_stream == NULL)
    {
        fprintf(stderr, "SDL audio disabled: %s\n", SDL_GetError());
        return false;
    }

    if (!SDL_ResumeAudioStreamDevice(ctx->audio_stream))
    {
        fprintf(stderr, "SDL audio resume failed: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(ctx->audio_stream);
        ctx->audio_stream = NULL;
        return false;
    }

    return true;
}

static bool sdl_shell_init_video(SdlShellContext *ctx)
{
    EngineVideoFrame frame = engine_video_get_frame();
    const char *renderer_name;
    int vsync = 0;

    if (frame.rgba == NULL || frame.width <= 0 || frame.height <= 0)
    {
        fprintf(stderr, "Engine framebuffer is unavailable\n");
        return false;
    }

    SDL_SetHint("SDL_RENDER_SCALE_QUALITY", "0");

    ctx->window = SDL_CreateWindow(
        SDL_SHELL_WINDOW_TITLE,
        frame.width * ctx->scale,
        frame.height * ctx->scale,
        0);
    if (ctx->window == NULL)
    {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    ctx->renderer = SDL_CreateRenderer(ctx->window, NULL);
    if (ctx->renderer == NULL)
    {
        ctx->renderer = SDL_CreateRenderer(ctx->window, SDL_SOFTWARE_RENDERER);
    }
    if (ctx->renderer == NULL)
    {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    renderer_name = SDL_GetRendererName(ctx->renderer);
    if (renderer_name == NULL)
        renderer_name = "unknown";
    if (!SDL_GetRenderVSync(ctx->renderer, &vsync))
        vsync = -1;
    fprintf(stderr, "SDL renderer: %s (vsync=%d)\n", renderer_name, vsync);

    SDL_SetRenderLogicalPresentation(ctx->renderer, frame.width, frame.height, SDL_LOGICAL_PRESENTATION_STRETCH);
    ctx->texture = SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, frame.width, frame.height);
    if (ctx->texture == NULL)
    {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(ctx->texture, SDL_SCALEMODE_NEAREST);

    return true;
}

static void sdl_shell_update_audio(SdlShellContext *ctx)
{
    EngineAudioBuffer buffer;
    size_t i;

    if (ctx->audio_stream == NULL)
        return;

    if (sdl_shell_is_fast_forward_active(ctx))
    {
        SDL_ClearAudioStream(ctx->audio_stream);
        return;
    }

    buffer = engine_audio_get_buffer();
    if (buffer.samples == NULL || buffer.sample_count == 0)
        return;

    if (!ctx->audio_activity_marked)
    {
        for (i = 0; i < buffer.sample_count; i++)
        {
            if (buffer.samples[i] != 0)
            {
                ctx->audio_activity_marked = true;
                sdl_shell_write_marker("build/sdl_audio_sdl_nonzero.ok", "SDL shell observed nonzero interleaved audio samples\n");
                fprintf(stderr, "SDL audio activity detected (%zu samples)\n", buffer.sample_count);
                break;
            }
        }
    }

    if (SDL_GetAudioStreamQueued(ctx->audio_stream) > SDL_SHELL_MAX_AUDIO_QUEUE_BYTES)
        SDL_ClearAudioStream(ctx->audio_stream);

    SDL_PutAudioStreamData(ctx->audio_stream, buffer.samples, (int)(buffer.sample_count * sizeof(int16_t)));
}

static bool sdl_shell_update_video(SdlShellContext *ctx)
{
    EngineVideoFrame frame = engine_video_get_frame();
    void *pixels;
    int pitch;
    int y;

    if (frame.rgba == NULL || frame.width <= 0 || frame.height <= 0)
        return false;

    if (!SDL_LockTexture(ctx->texture, NULL, &pixels, &pitch))
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
    SDL_RenderTexture(ctx->renderer, ctx->texture, NULL, NULL);
    SDL_RenderPresent(ctx->renderer);
    return true;
}

static void sdl_shell_handle_event(SdlShellContext *ctx, const SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        ctx->running = false;
        return;
    }

    if (event->type == SDL_EVENT_WINDOW_FOCUS_GAINED)
    {
        ctx->has_focus = true;
        return;
    }

    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST)
    {
        ctx->has_focus = false;
        ctx->fast_forward_active = false;
        return;
    }

    if (event->type == SDL_EVENT_GAMEPAD_ADDED)
    {
        if (ctx->gamepad == NULL)
            sdl_shell_open_gamepad(ctx, event->gdevice.which);
        return;
    }

    if (event->type == SDL_EVENT_GAMEPAD_REMOVED)
    {
        if (ctx->gamepad != NULL && SDL_GetGamepadID(ctx->gamepad) == event->gdevice.which)
        {
            fprintf(stderr, "SDL gamepad disconnected\n");
            sdl_shell_close_gamepad(ctx);
            sdl_shell_open_first_gamepad(ctx);
        }
        return;
    }

    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
    {
        uint16_t button = sdl_shell_key_to_gba_button(event->key.scancode);

        if (event->key.scancode == SDL_SCANCODE_TAB)
        {
            if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat && (event->key.mod & SDL_KMOD_SHIFT))
            {
                ctx->fast_forward_toggled = !ctx->fast_forward_toggled;
                ctx->fast_forward_active = false;
            }
            else if (!(event->key.mod & SDL_KMOD_SHIFT))
            {
                ctx->fast_forward_active = (event->type == SDL_EVENT_KEY_DOWN);
            }

            return;
        }

        if (button != 0)
        {
            if (event->type == SDL_EVENT_KEY_DOWN)
            {
                ctx->input_state |= button;
                ctx->input_latched |= button;
            }
            else
                ctx->input_state &= (uint16_t)~button;
        }
    }

    if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event->type == SDL_EVENT_GAMEPAD_BUTTON_UP)
    {
        uint16_t button = sdl_shell_gamepad_button_to_gba_button(event->gbutton.button);

        if (button != 0)
        {
            if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
            {
                ctx->input_state |= button;
                ctx->input_latched |= button;
            }
            else
                ctx->input_state &= (uint16_t)~button;
        }
    }

    if (event->type != SDL_EVENT_KEY_DOWN || event->key.repeat)
        return;

    switch (event->key.key)
    {
    case SDLK_ESCAPE:
        ctx->running = false;
        break;
    case SDLK_F1:
        engine_open_start_menu();
        break;
    case SDLK_F5:
        engine_save_state(ctx->state_path);
        // Also export a raw battery-backed flash image so mGBA can import.
        // Note: this reflects the latest in-game flash contents; mGBA
        // cannot restore an emulator snapshot like FRSTATE1.
        {
            char export_path[1024];
            snprintf(export_path, sizeof(export_path), "%s.sav", ctx->state_path);
            PortableFlash_Export(export_path);
            fprintf(stderr, "Exported mGBA save: %s\n", export_path);
        }
        break;
    case SDLK_F9:
        if (engine_load_state(ctx->state_path) != 0)
            sdl_shell_update_video(ctx);
        break;
    case SDLK_F10:
        m4aSongNumStart(SE_BIKE_BELL);
        break;
    default:
        break;
    }
}

static void sdl_shell_shutdown(SdlShellContext *ctx)
{
    sdl_shell_close_gamepad(ctx);
    if (ctx->audio_stream != NULL)
        SDL_DestroyAudioStream(ctx->audio_stream);
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
    SdlShellArgs args;
    uint8_t *rom_data;
    size_t rom_size;
    Uint64 perf_freq;
    double target_frame_ms;
    Uint64 fps_window_start;
    unsigned int fps_frames;

    if (!sdl_shell_parse_args(argc, argv, &args))
    {
        sdl_shell_print_usage(argv[0]);
        return 1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.scale = SDL_SHELL_DEFAULT_SCALE;
    ctx.state_path = args.state_path;
    ctx.has_focus = true;
    ctx.audio_selftest_enabled = (getenv("FIRERED_AUDIO_SELFTEST") != NULL);
    ctx.running = true;
    perf_freq = SDL_GetPerformanceFrequency();
    target_frame_ms = 1000.0 / SDL_SHELL_TARGET_FPS;
    fps_window_start = SDL_GetTicks();
    fps_frames = 0;

    rom_data = sdl_shell_read_file(args.rom_path, &rom_size);
    if (rom_data == NULL)
    {
        fprintf(stderr, "Failed to read base ROM (check path and permissions): %s\n", args.rom_path);
        return 1;
    }

    if (args.bps_path != NULL)
    {
        char errbuf[512];
        uint8_t *patch_data;
        size_t patch_size;
        uint8_t *patched_rom = NULL;
        size_t patched_size = 0;

        fprintf(stderr, "Runtime mod: applying BPS \"%s\" (in memory; base ROM on disk is not modified)\n", args.bps_path);
        patch_data = sdl_shell_read_file(args.bps_path, &patch_size);
        if (patch_data == NULL)
        {
            fprintf(stderr, "Failed to read BPS patch: %s\n", args.bps_path);
            free(rom_data);
            return 1;
        }

        if (!mod_patch_bps_apply(
                rom_data,
                rom_size,
                patch_data,
                patch_size,
                &patched_rom,
                &patched_size,
                errbuf,
                sizeof(errbuf)))
        {
            fprintf(stderr, "BPS apply failed: %s\n", errbuf[0] != '\0' ? errbuf : "(no details)");
            free(patch_data);
            free(rom_data);
            return 1;
        }

        free(patch_data);
        free(rom_data);
        rom_data = patched_rom;
        rom_size = patched_size;
    }

    engine_set_core(firered_core_get());
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
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
    sdl_shell_open_first_gamepad(&ctx);

    while (ctx.running)
    {
        SDL_Event event;
        Uint64 frame_start;
        Uint64 frame_end;
        double elapsed_ms;

        frame_start = SDL_GetPerformanceCounter();
        if (ctx.audio_selftest_enabled && ctx.frame_count == 0)
            SDL_SetWindowTitle(ctx.window, "decomp-engine SDL shell [loop-start]");

        while (SDL_PollEvent(&event))
            sdl_shell_handle_event(&ctx, &event);

        engine_input_set_buttons(sdl_shell_collect_input(&ctx));
        if (ctx.audio_selftest_enabled && ctx.frame_count == 0)
            SDL_SetWindowTitle(ctx.window, "decomp-engine SDL shell [pre-frame-1]");
        engine_run_frame();
        ctx.frame_count += 1;
        ctx.input_latched = 0;

        if (ctx.audio_selftest_enabled && ctx.frame_count == 1)
            SDL_SetWindowTitle(ctx.window, "decomp-engine SDL shell [post-frame-1]");

        if (ctx.audio_selftest_enabled && ctx.frame_count == 1)
        {
            sdl_shell_write_marker("build/sdl_audio_loop.ok", "entered SDL frame loop\n");
        }

        if (ctx.audio_selftest_enabled && ctx.frame_count == 5)
        {
            m4aSongNumStart(SE_BIKE_BELL);
            fprintf(stderr, "SDL audio self-test: triggered SE_BIKE_BELL on frame %u\n", ctx.frame_count);
            sdl_shell_write_marker("build/sdl_audio_selftest_triggered.ok", "triggered SE_BIKE_BELL from SDL shell\n");
        }

        if (!sdl_shell_update_video(&ctx))
            break;
        sdl_shell_update_audio(&ctx);

        frame_end = SDL_GetPerformanceCounter();
        elapsed_ms = (double)(frame_end - frame_start) * 1000.0 / (double)perf_freq;
        if (!sdl_shell_is_fast_forward_active(&ctx) && elapsed_ms < target_frame_ms)
            SDL_Delay((Uint32)(target_frame_ms - elapsed_ms));

        fps_frames += 1;
        {
            Uint64 now = SDL_GetTicks();
            Uint64 elapsed = now - fps_window_start;

            if (elapsed >= 500)
            {
                char title[128];
                double fps = (double)fps_frames * 1000.0 / (double)elapsed;

                snprintf(
                    title,
                    sizeof(title),
                    "%s%s - %.1f FPS",
                    SDL_SHELL_WINDOW_TITLE,
                    sdl_shell_is_fast_forward_active(&ctx) ? " [FF]" : "",
                    fps);
                SDL_SetWindowTitle(ctx.window, title);
                fps_window_start = now;
                fps_frames = 0;
            }
        }
    }

    sdl_shell_shutdown(&ctx);
    return 0;
}
