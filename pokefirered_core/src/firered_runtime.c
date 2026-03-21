#include "../../engine/core/engine_backend.h"

int firered_init(const uint8_t *rom, size_t rom_size)
{
    return engine_backend_init(rom, rom_size);
}

void firered_reset(void)
{
    engine_backend_reset();
}

void firered_set_input(uint16_t buttons)
{
    engine_backend_set_input(buttons);
}

void firered_run_frame(void)
{
    engine_backend_run_frame();
}

int firered_open_start_menu(void)
{
    return engine_backend_open_start_menu();
}

int firered_save_state(const char *path)
{
    return engine_backend_save_state(path);
}

int firered_load_state(const char *path)
{
    return engine_backend_load_state(path);
}

const uint8_t *firered_get_framebuffer_rgba(void)
{
    return engine_backend_get_framebuffer_rgba();
}

int firered_get_frame_width(void)
{
    return engine_backend_get_frame_width();
}

int firered_get_frame_height(void)
{
    return engine_backend_get_frame_height();
}

int16_t *firered_get_audio_samples(size_t *count)
{
    return engine_backend_get_audio_samples(count);
}

void firered_shutdown(void)
{
    engine_backend_shutdown();
}

void firered_runtime_vblank_wait(void)
{
    engine_backend_vblank_wait();
}

void firered_runtime_request_soft_reset(void)
{
    engine_backend_request_soft_reset();
}

void firered_runtime_trace_external(const char *message)
{
#ifdef NDEBUG
    (void)message;
#else
    engine_backend_trace_external(message);
#endif
}

unsigned long firered_runtime_get_completed_frame_external(void)
{
    return engine_backend_get_completed_frame_external();
}

void firered_runtime_trace_bytes_external(const char *label, const void *src, const void *dst)
{
#ifdef NDEBUG
    (void)label;
    (void)src;
    (void)dst;
#else
    engine_backend_trace_bytes_external(label, src, dst);
#endif
}
