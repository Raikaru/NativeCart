#include "../../engine/core/engine_internal.h"

void firered_audio_reset(void)
{
    engine_audio_reset();
}

int16_t *firered_audio_get_samples(size_t *count)
{
    return engine_audio_get_samples(count);
}
