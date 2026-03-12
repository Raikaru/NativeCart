#ifndef ENGINE_CORE_ENGINE_RUNTIME_INTERNAL_H
#define ENGINE_CORE_ENGINE_RUNTIME_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

void engine_backend_vblank_wait(void);
void engine_backend_request_soft_reset(void);

#ifdef __cplusplus
}
#endif

#endif
