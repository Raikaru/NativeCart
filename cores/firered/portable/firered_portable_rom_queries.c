#include "global.h"
#include "engine_internal.h"

#include "portable/firered_portable_rom_queries.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_queries_invalidate_cache(void)
{
}

u32 firered_portable_rom_crc32_ieee_full(void)
{
    return 0u;
}

u8 firered_portable_rom_queries_copy_cart_title12(u8 *out12)
{
    (void)out12;
    return 0u;
}

u8 firered_portable_rom_queries_copy_game_code4(u8 *out4)
{
    (void)out4;
    return 0u;
}

u8 firered_portable_rom_queries_software_version(void)
{
    return 0u;
}

#else

static u32 s_cached_crc;
static u8 s_crc_valid;

static uint32_t rom_crc32_ieee(const uint8_t *data, size_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;
    int j;

    for (i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return crc ^ 0xFFFFFFFFu;
}

void firered_portable_rom_queries_invalidate_cache(void)
{
    s_crc_valid = 0;
}

u32 firered_portable_rom_crc32_ieee_full(void)
{
    size_t rom_size;
    const uint8_t *rom;

    if (s_crc_valid)
        return s_cached_crc;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const uint8_t *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size == 0 || rom == NULL)
    {
        s_cached_crc = 0u;
        s_crc_valid = 1;
        return s_cached_crc;
    }

    s_cached_crc = rom_crc32_ieee(rom, rom_size);
    s_crc_valid = 1;
    return s_cached_crc;
}

u8 firered_portable_rom_queries_copy_cart_title12(u8 *out12)
{
    size_t rom_size;
    const uint8_t *rom;

    if (out12 == NULL)
        return 0u;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const uint8_t *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < 0xACu || rom == NULL)
        return 0u;

    memcpy(out12, rom + 0xA0u, 12u);
    return 12u;
}

u8 firered_portable_rom_queries_copy_game_code4(u8 *out4)
{
    size_t rom_size;
    const uint8_t *rom;

    if (out4 == NULL)
        return 0u;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const uint8_t *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < 0xB0u || rom == NULL)
        return 0u;

    memcpy(out4, rom + 0xACu, 4u);
    return 4u;
}

u8 firered_portable_rom_queries_software_version(void)
{
    size_t rom_size;
    const uint8_t *rom;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const uint8_t *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < 0xBDu || rom == NULL)
        return 0u;

    return rom[0xBCu];
}

#endif /* PORTABLE */
