#include "../../engine/core/engine_internal.h"

void firered_input_reset(void)
{
    engine_backend_input_reset();
}

void firered_input_set_buttons(uint16_t buttons)
{
    engine_backend_input_set_buttons(buttons);
}
