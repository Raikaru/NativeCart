#include "../../engine/core/engine_internal.h"

int firered_memory_init(const uint8_t *rom, size_t rom_size)
{
    return engine_memory_init(rom, rom_size);
}

void firered_memory_shutdown(void)
{
    engine_memory_shutdown();
}
