#ifndef FIRERED_RUNTIME_INTERNAL_H
#define FIRERED_RUNTIME_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../engine/core/engine_runtime_internal.h"

void firered_runtime_vblank_wait(void);
void firered_runtime_request_soft_reset(void);

#ifdef __cplusplus
}
#endif

#endif
