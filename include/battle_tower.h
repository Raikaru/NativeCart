#ifndef GUARD_BATTLE_TOWER_H
#define GUARD_BATTLE_TOWER_H

#include "global.h"
#include "constants/battle_tower.h"

#define BATTLE_TOWER_EREADER_TRAINER_ID 200
#define BATTLE_TOWER_RECORD_MIXING_TRAINER_BASE_ID 100

struct BattleTowerTrainer
{
    u8 trainerClass;
    u8 name[3];
    u8 teamFlags;
    u16 greeting[6];
};

struct BattleTowerPokemonTemplate
{
    u16 species;
    u8 heldItem;
    u8 teamFlags;
    u16 moves[4];
    u8 evSpread;
    u8 nature;
};

extern const struct BattleTowerPokemonTemplate gBattleTowerLevel50Mons_Compiled[];
extern const struct BattleTowerPokemonTemplate gBattleTowerLevel100Mons_Compiled[];

#ifdef PORTABLE
/* NULL => use compiled tables (see macros below). Set by ROM refresh. */
extern const struct BattleTowerPokemonTemplate *gBattleTowerLevel50MonsActive;
extern const struct BattleTowerPokemonTemplate *gBattleTowerLevel100MonsActive;
#define gBattleTowerLevel50Mons  ((gBattleTowerLevel50MonsActive) != NULL ? (gBattleTowerLevel50MonsActive) : (gBattleTowerLevel50Mons_Compiled))
#define gBattleTowerLevel100Mons ((gBattleTowerLevel100MonsActive) != NULL ? (gBattleTowerLevel100MonsActive) : (gBattleTowerLevel100Mons_Compiled))
#else
#define gBattleTowerLevel50Mons  gBattleTowerLevel50Mons_Compiled
#define gBattleTowerLevel100Mons gBattleTowerLevel100Mons_Compiled
#endif

extern const u16 gBattleTowerBannedSpecies[];

void ClearEReaderTrainer(struct BattleTowerEReaderTrainer *);
void ValidateEReaderTrainer(void);
u8 GetBattleTowerTrainerFrontSpriteId(void);
u8 GetEreaderTrainerFrontSpriteId(void);
void CopyEReaderTrainerName5(u8 *dest);
void GetBattleTowerTrainerName(u8 *text);
u8 GetEreaderTrainerClassId(void);
u8 GetBattleTowerTrainerClassNameId(void);

#endif //GUARD_BATTLE_TOWER_H
