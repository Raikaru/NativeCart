#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#define POKEMON_NAME_LENGTH 10
#define PLAYER_NAME_LENGTH 7
#define MAX_MON_MOVES 4

struct PokemonSubstruct0 { uint16_t species; uint16_t heldItem; uint32_t experience; uint8_t ppBonuses; uint8_t friendship; uint16_t filler; };
struct PokemonSubstruct1 { uint16_t moves[MAX_MON_MOVES]; uint8_t pp[MAX_MON_MOVES]; };
struct PokemonSubstruct2 { uint8_t hpEV; uint8_t attackEV; uint8_t defenseEV; uint8_t speedEV; uint8_t spAttackEV; uint8_t spDefenseEV; uint8_t cool; uint8_t beauty; uint8_t cute; uint8_t smart; uint8_t tough; uint8_t sheen; };
struct PokemonSubstruct3 { uint8_t pokerus; uint8_t metLocation; uint16_t metLevel:7; uint16_t metGame:4; uint16_t pokeball:4; uint16_t otGender:1; uint32_t hpIV:5; uint32_t attackIV:5; uint32_t defenseIV:5; uint32_t speedIV:5; uint32_t spAttackIV:5; uint32_t spDefenseIV:5; uint32_t isEgg:1; uint32_t abilityNum:1; uint32_t coolRibbon:3; uint32_t beautyRibbon:3; uint32_t cuteRibbon:3; uint32_t smartRibbon:3; uint32_t toughRibbon:3; uint32_t championRibbon:1; uint32_t winningRibbon:1; uint32_t victoryRibbon:1; uint32_t artistRibbon:1; uint32_t effortRibbon:1; uint32_t marineRibbon:1; uint32_t landRibbon:1; uint32_t skyRibbon:1; uint32_t countryRibbon:1; uint32_t nationalRibbon:1; uint32_t earthRibbon:1; uint32_t worldRibbon:1; uint32_t unusedRibbons:4; uint32_t modernFatefulEncounter:1; };

#define NUM_SUBSTRUCT_BYTES 12

union PokemonSubstruct { struct PokemonSubstruct0 type0; struct PokemonSubstruct1 type1; struct PokemonSubstruct2 type2; struct PokemonSubstruct3 type3; uint16_t raw[NUM_SUBSTRUCT_BYTES / 2]; };

struct BoxPokemon { uint32_t personality; uint32_t otId; uint8_t nickname[POKEMON_NAME_LENGTH]; uint8_t language; uint8_t isBadEgg:1; uint8_t hasSpecies:1; uint8_t isEgg:1; uint8_t blockBoxRS:1; uint8_t unused:4; uint8_t otName[PLAYER_NAME_LENGTH]; uint8_t markings; uint16_t checksum; uint16_t unknown; union { uint32_t raw[(NUM_SUBSTRUCT_BYTES * 4) / 4]; union PokemonSubstruct substructs[4]; } secure; };

#define TOTAL_BOXES_COUNT 14
#define IN_BOX_COUNT 30
#define BOX_NAME_LENGTH 8

struct PokemonStorage { uint8_t currentBox; struct BoxPokemon boxes[TOTAL_BOXES_COUNT][IN_BOX_COUNT]; uint8_t boxNames[TOTAL_BOXES_COUNT][BOX_NAME_LENGTH + 1]; uint8_t boxWallpapers[TOTAL_BOXES_COUNT]; };

int main() {
    printf("sizeof(struct BoxPokemon) = %zu\n", sizeof(struct BoxPokemon));
    printf("sizeof(struct PokemonStorage) = %zu\n", sizeof(struct PokemonStorage));
    printf("offsetof boxNames[0] = %zu (0x%zX)\n", offsetof(struct PokemonStorage, boxNames[0]), offsetof(struct PokemonStorage, boxNames[0]));
    printf("offsetof boxWallpapers[0] = %zu (0x%zX)\n", offsetof(struct PokemonStorage, boxWallpapers[0]), offsetof(struct PokemonStorage, boxWallpapers[0]));
    printf("offsetof boxes = %zu (0x%zX)\n", offsetof(struct PokemonStorage, boxes), offsetof(struct PokemonStorage, boxes));
    return 0;
}
