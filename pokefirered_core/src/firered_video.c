#include "../../engine/core/engine_internal.h"

void firered_video_reset(void)
{
    engine_video_reset();
}

void firered_video_render_frame(void)
{
    engine_video_render_frame();
}

const uint8_t *firered_video_get_framebuffer(void)
{
    return engine_video_get_framebuffer();
}
