#include "engine_input.h"

#include "core_loader.h"

void engine_input_set_buttons(uint16_t buttons)
{
    GameCore *core = engine_get_core();

    if (core != NULL && core->set_input != NULL)
        core->set_input(buttons);
}
