#include "global.h"
#include "main.h"
#include "engine_internal.h"

#include "portable/firered_portable_rom_header.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Initial values match the pre-sync portable build (vanilla); overwritten from
 * mapped ROM by firered_portable_sync_rom_header_from_cartridge() after load.
 */
#if defined(FIRERED)
char RomHeaderGameCode[GAME_CODE_LENGTH] = { 'B', 'P', 'R', 'E' };
#elif defined(LEAFGREEN)
char RomHeaderGameCode[GAME_CODE_LENGTH] = { 'B', 'P', 'G', 'E' };
#else
char RomHeaderGameCode[GAME_CODE_LENGTH] = { 'U', 'N', 'K', 'N' };
#endif

char RomHeaderSoftwareVersion = (char)GAME_VERSION;

void firered_portable_sync_rom_header_from_cartridge(void) {
    size_t rom_size;
    const uint8_t *rom;

    rom_size = engine_memory_get_loaded_rom_size();
    /* Game code @ 0xAC (4 bytes); software version @ 0xBC (1 byte). */
    if (rom_size < 0xBDu) {
        return;
    }

    rom = (const uint8_t *)(uintptr_t)ENGINE_ROM_ADDR;
    memcpy(RomHeaderGameCode, rom + 0xACu, (size_t)GAME_CODE_LENGTH);
    RomHeaderSoftwareVersion = (char)rom[0xBCu];
}
