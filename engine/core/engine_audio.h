#ifndef ENGINE_CORE_ENGINE_AUDIO_H
#define ENGINE_CORE_ENGINE_AUDIO_H

#include "../interfaces/audio_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

EngineAudioBuffer engine_audio_get_buffer(void);

#ifdef __cplusplus
}
#endif

#endif
