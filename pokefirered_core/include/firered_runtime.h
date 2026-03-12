#ifndef FIRERED_RUNTIME_H
#define FIRERED_RUNTIME_H

#include "../../engine/core/engine_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

int firered_init(const uint8_t *rom, size_t rom_size);
void firered_reset(void);
void firered_set_input(uint16_t buttons);
void firered_run_frame(void);
int firered_open_start_menu(void);
int firered_save_state(const char *path);
int firered_load_state(const char *path);
const uint8_t *firered_get_framebuffer_rgba(void);
int firered_get_frame_width(void);
int firered_get_frame_height(void);
int16_t *firered_get_audio_samples(size_t *count);
void firered_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
