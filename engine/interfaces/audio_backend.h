#ifndef ENGINE_INTERFACES_AUDIO_BACKEND_H
#define ENGINE_INTERFACES_AUDIO_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EngineAudioBuffer {
    int16_t *samples;
    size_t sample_count;
} EngineAudioBuffer;

#ifdef __cplusplus
}
#endif

#endif
