#ifndef ENGINE_CORE_CORE_LOADER_H
#define ENGINE_CORE_CORE_LOADER_H

#include "../interfaces/game_core.h"

#ifdef __cplusplus
extern "C" {
#endif

void engine_set_core(GameCore *core);
GameCore *engine_get_core(void);

#ifdef __cplusplus
}
#endif

#endif
