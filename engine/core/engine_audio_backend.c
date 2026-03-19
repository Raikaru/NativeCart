#include "engine_internal.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    ENGINE_AUDIO_MAX_INTERLEAVED_SAMPLES = 4096,
};

static int16_t g_audio_buffer[ENGINE_AUDIO_MAX_INTERLEAVED_SAMPLES];
static size_t g_audio_sample_count;
static int g_audio_backend_nonzero_marked;

static void engine_audio_write_marker(const char *path, const char *contents) {
    FILE *file;

    if (path == NULL || contents == NULL) {
        return;
    }

    file = fopen(path, "wb");
    if (file == NULL) {
        return;
    }

    fputs(contents, file);
    fclose(file);
}

void engine_audio_reset(void) {
    memset(g_audio_buffer, 0, sizeof(g_audio_buffer));
    g_audio_sample_count = 0;
    g_audio_backend_nonzero_marked = 0;
}

int16_t *engine_audio_get_samples(size_t *count) {
    if (count != NULL) {
        *count = g_audio_sample_count;
    }
    return g_audio_buffer;
}

void engine_audio_submit_pcm(const int16_t *samples, size_t count) {
    if (samples == NULL || count == 0) {
        g_audio_sample_count = 0;
        return;
    }

    if (count > ENGINE_AUDIO_MAX_INTERLEAVED_SAMPLES) {
        count = ENGINE_AUDIO_MAX_INTERLEAVED_SAMPLES;
    }

    memcpy(g_audio_buffer, samples, count * sizeof(*samples));
    g_audio_sample_count = count;
}

void engine_audio_submit_gba_directsound(const int8_t *fifo_a, const int8_t *fifo_b, size_t sample_count) {
    size_t i;
    size_t max_stereo_samples = ENGINE_AUDIO_MAX_INTERLEAVED_SAMPLES / 2;

    if (fifo_a == NULL || fifo_b == NULL || sample_count == 0) {
        g_audio_sample_count = 0;
        return;
    }

    if (sample_count > max_stereo_samples) {
        sample_count = max_stereo_samples;
    }

    for (i = 0; i < sample_count; ++i) {
        if (!g_audio_backend_nonzero_marked
         && getenv("FIRERED_AUDIO_SELFTEST") != NULL
         && (fifo_a[i] != 0 || fifo_b[i] != 0)) {
            g_audio_backend_nonzero_marked = 1;
            engine_audio_write_marker("build/portable_audio_backend_nonzero.ok",
                                      "engine audio backend received nonzero DirectSound samples\n");
        }
        g_audio_buffer[i * 2 + 0] = (int16_t)fifo_b[i] << 8;
        g_audio_buffer[i * 2 + 1] = (int16_t)fifo_a[i] << 8;
    }

    g_audio_sample_count = sample_count * 2;
}
