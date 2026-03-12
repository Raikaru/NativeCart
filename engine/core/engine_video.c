#include "engine_video.h"

#include "core_loader.h"

EngineVideoFrame engine_video_get_frame(void)
{
    EngineVideoFrame frame = {0};
    GameCore *core = engine_get_core();

    if (core == NULL || core->get_framebuffer == NULL || core->get_frame_width == NULL || core->get_frame_height == NULL)
        return frame;

    frame.rgba = core->get_framebuffer();
    frame.width = core->get_frame_width();
    frame.height = core->get_frame_height();
    return frame;
}
