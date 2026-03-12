#include "global.h"
#include "main.h"

#if defined(FIRERED)
const char RomHeaderGameCode[GAME_CODE_LENGTH] = { 'B', 'P', 'R', 'E' };
#elif defined(LEAFGREEN)
const char RomHeaderGameCode[GAME_CODE_LENGTH] = { 'B', 'P', 'G', 'E' };
#else
const char RomHeaderGameCode[GAME_CODE_LENGTH] = { 'U', 'N', 'K', 'N' };
#endif

const char RomHeaderSoftwareVersion = GAME_VERSION;
