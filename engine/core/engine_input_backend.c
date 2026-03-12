#include "engine_internal.h"

#include <stdint.h>

void engine_backend_input_reset(void) {
    *(volatile uint16_t *)(uintptr_t)ENGINE_REG_KEYINPUT = 0x03FFu;
}

void engine_backend_input_set_buttons(uint16_t buttons) {
    uint16_t raw = (uint16_t)(~buttons) & 0x03FFu;
    *(volatile uint16_t *)(uintptr_t)ENGINE_REG_KEYINPUT = raw;
}
