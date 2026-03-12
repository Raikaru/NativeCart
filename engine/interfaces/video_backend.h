#ifndef ENGINE_INTERFACES_VIDEO_BACKEND_H
#define ENGINE_INTERFACES_VIDEO_BACKEND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EngineVideoFrame {
    const uint8_t *rgba;
    int width;
    int height;
} EngineVideoFrame;

#ifdef __cplusplus
}
#endif

#endif
