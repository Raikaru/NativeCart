#ifndef ENGINE_CORE_ENGINE_VIDEO_H
#define ENGINE_CORE_ENGINE_VIDEO_H

#include "../interfaces/video_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

EngineVideoFrame engine_video_get_frame(void);

#ifdef __cplusplus
}
#endif

#endif
