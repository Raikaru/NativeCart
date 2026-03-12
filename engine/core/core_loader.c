#include "core_loader.h"

static GameCore *g_active_core = NULL;

void engine_set_core(GameCore *core)
{
    g_active_core = core;
}

GameCore *engine_get_core(void)
{
    return g_active_core;
}
