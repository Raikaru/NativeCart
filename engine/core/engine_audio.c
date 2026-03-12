#include "engine_audio.h"

#include "core_loader.h"

EngineAudioBuffer engine_audio_get_buffer(void)
{
    EngineAudioBuffer buffer = {0};
    GameCore *core = engine_get_core();

    if (core == NULL || core->get_audio_samples == NULL)
        return buffer;

    buffer.samples = core->get_audio_samples(&buffer.sample_count);
    return buffer;
}
