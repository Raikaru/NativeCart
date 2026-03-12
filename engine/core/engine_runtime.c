#include "engine_runtime.h"

#include "core_loader.h"

int engine_load_rom(const uint8_t *rom, size_t rom_size)
{
    GameCore *core = engine_get_core();

    if (core == NULL || core->load_rom == NULL)
        return 0;

    if (core->init != NULL)
        core->init();

    return core->load_rom(rom, rom_size);
}

void engine_shutdown(void)
{
    GameCore *core = engine_get_core();

    if (core != NULL && core->shutdown != NULL)
        core->shutdown();
}

void engine_reset(void)
{
    GameCore *core = engine_get_core();

    if (core != NULL && core->reset != NULL)
        core->reset();
}

void engine_run_frame(void)
{
    GameCore *core = engine_get_core();

    if (core != NULL && core->step_frame != NULL)
        core->step_frame();
}

int engine_open_start_menu(void)
{
    GameCore *core = engine_get_core();

    if (core == NULL || core->open_start_menu == NULL)
        return 0;

    return core->open_start_menu();
}

int engine_save_state(const char *path)
{
    GameCore *core = engine_get_core();

    if (core == NULL || core->save_state == NULL)
        return 0;

    return core->save_state(path);
}

int engine_load_state(const char *path)
{
    GameCore *core = engine_get_core();

    if (core == NULL || core->load_state == NULL)
        return 0;

    return core->load_state(path);
}
