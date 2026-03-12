#ifndef ENGINE_CORE_ENGINE_BACKEND_H
#define ENGINE_CORE_ENGINE_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int engine_backend_init(const uint8_t *rom, size_t rom_size);
void engine_backend_reset(void);
void engine_backend_set_input(uint16_t buttons);
void engine_backend_run_frame(void);
int engine_backend_open_start_menu(void);
int engine_backend_save_state(const char *path);
int engine_backend_load_state(const char *path);
const uint8_t *engine_backend_get_framebuffer_rgba(void);
int engine_backend_get_frame_width(void);
int engine_backend_get_frame_height(void);
int16_t *engine_backend_get_audio_samples(size_t *count);
void engine_backend_shutdown(void);
void engine_backend_vblank_wait(void);
void engine_backend_request_soft_reset(void);
void engine_backend_trace_external(const char *message);
unsigned long engine_backend_get_completed_frame_external(void);
void engine_backend_trace_bytes_external(const char *label, const void *src, const void *dst);

#ifdef __cplusplus
}
#endif

#endif
