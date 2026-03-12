#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int16_t g_audio_buffer[2048];

void engine_audio_reset(void) {
    memset(g_audio_buffer, 0, sizeof(g_audio_buffer));
}

int16_t *engine_audio_get_samples(size_t *count) {
    if (count != NULL) {
        *count = 0;
    }
    return g_audio_buffer;
}
