#ifndef ENGINE_INTERFACES_GAME_CORE_H
#define ENGINE_INTERFACES_GAME_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GameCore {
    const char *name;
    void (*init)(void);
    void (*shutdown)(void);
    int (*load_rom)(const uint8_t *data, size_t size);
    void (*reset)(void);
    void (*set_input)(uint16_t buttons);
    void (*step_frame)(void);
    int (*open_start_menu)(void);
    int (*save_state)(const char *path);
    int (*load_state)(const char *path);
    const uint8_t *(*get_framebuffer)(void);
    int (*get_frame_width)(void);
    int (*get_frame_height)(void);
    int16_t *(*get_audio_samples)(size_t *count);
} GameCore;

#ifdef __cplusplus
}
#endif

#endif
