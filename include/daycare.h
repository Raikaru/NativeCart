#ifndef GUARD_DAYCARE_H
#define GUARD_DAYCARE_H

#include "global.h"

u8 *GetMonNick(struct Pokemon *mon, u8 *dest);
u8 *GetBoxMonNick(struct BoxPokemon *mon, u8 *dest);
u8 CountPokemonInDaycare(struct DayCare *daycare);
void InitDaycareMailRecordMixing(struct DayCare *daycare, struct RecordMixingDayCareMail *daycareMail);
void StoreSelectedPokemonInDaycare(void);
u16 TakePokemonFromDaycare(void);
void GetDaycareCost(void);
u8 GetNumLevelsGainedFromDaycare(void);
void TriggerPendingDaycareEgg(void);
void RejectEggFromDayCare(void);
void CreateEgg(struct Pokemon *mon, u16 species, bool8 setHotSpringsLocation);
void GiveEggFromDaycare(void);
void Daycare_BuildEggMoveset(struct Pokemon *egg, struct BoxPokemon *father, struct BoxPokemon *mother);
u16 Daycare_GetEggSpecies(u16 species);
bool8 DoEggActions_CheckHatch(void);
u16 GetSelectedMonNickAndSpecies(void);
void GetDaycareMonNicknames(void);
u8 GetDaycareState(void);
void SetDaycareCompatibilityString(void);
bool8 NameHasGenderSymbol(const u8 *name, u8 genderRatio);
void ShowDaycareLevelMenu(void);
void ChooseSendDaycareMon(void);

void ScriptHatchMon(void);
/* Used by daycare_egg_hatch.c (split from daycare.c for agbcc). */
u8 *DayCare_GetMonNickname(struct Pokemon *mon, u8 *dest);
void AddHatchedMonToParty(u8 id);
void EggHatch(void);
u8 GetEggStepsToSubtract(void);
bool8 ShouldEggHatch(void);

#endif // GUARD_DAYCARE_H
