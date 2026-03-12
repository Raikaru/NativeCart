#ifndef ENGINE_CORE_ENGINE_RUNTIME_H
#define ENGINE_CORE_ENGINE_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int engine_load_rom(const uint8_t *rom, size_t rom_size);
void engine_shutdown(void);
void engine_reset(void);
void engine_run_frame(void);
int engine_open_start_menu(void);
int engine_save_state(const char *path);
int engine_load_state(const char *path);

#ifdef __cplusplus
}
#endif

#endif
