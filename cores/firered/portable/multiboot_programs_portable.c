#include "../../../include/global.h"

const u8 gMultiBootProgram_PokemonColosseum_Start[1] = {0};
const u8 gMultiBootProgram_EReader_Start[1] = {0};
const u8 gMultiBootProgram_BerryGlitchFix_Start[0x3BF4] = {0};

__asm__(
    ".global gMultiBootProgram_PokemonColosseum_End\n"
    "gMultiBootProgram_PokemonColosseum_End = gMultiBootProgram_PokemonColosseum_Start + 1\n"
    ".global gMultiBootProgram_EReader_End\n"
    "gMultiBootProgram_EReader_End = gMultiBootProgram_EReader_Start + 1\n"
    ".global gMultiBootProgram_BerryGlitchFix_End\n"
    "gMultiBootProgram_BerryGlitchFix_End = gMultiBootProgram_BerryGlitchFix_Start + 0x3BF4\n");
