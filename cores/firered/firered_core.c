#include "firered_core.h"

#include "../../engine/core/engine_backend.h"

static void firered_core_init(void)
{
}

static int firered_core_load_rom(const uint8_t *data, size_t size)
{
    return engine_backend_init(data, size);
}

static const uint8_t *firered_core_get_framebuffer(void)
{
    return engine_backend_get_framebuffer_rgba();
}

static GameCore g_firered_core = {
    "firered",
    firered_core_init,
    engine_backend_shutdown,
    firered_core_load_rom,
    engine_backend_reset,
    engine_backend_set_input,
    engine_backend_run_frame,
    engine_backend_open_start_menu,
    engine_backend_save_state,
    engine_backend_load_state,
    firered_core_get_framebuffer,
    engine_backend_get_frame_width,
    engine_backend_get_frame_height,
    engine_backend_get_audio_samples,
};

GameCore *firered_core_get(void)
{
    return &g_firered_core;
}
