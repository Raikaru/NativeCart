#include "global.h"

#include <stdint.h>

#define U16_LO(x) ((u8)((u16)(x) & 0xFF))
#define U16_HI(x) ((u8)(((u16)(x) >> 8) & 0xFF))
#define U32_B0(x) ((u8)((u32)(x) & 0xFF))
#define U32_B1(x) ((u8)(((u32)(x) >> 8) & 0xFF))
#define U32_B2(x) ((u8)(((u32)(x) >> 16) & 0xFF))
#define U32_B3(x) ((u8)(((u32)(x) >> 24) & 0xFF))
#define FIRERED_PORTABLE_PTR_TOKEN_BASE 0x82000000u

extern const u8 gFldEffScript_Ash[];
extern const u8 gFldEffScript_BerryTreeGrowthSparkle[];
extern const u8 gFldEffScript_BikeTireTracks[];
extern const u8 gFldEffScript_Bubbles[];
extern const u8 gFldEffScript_CutGrass[];
extern const u8 gFldEffScript_DeepSandFootprints[];
extern const u8 gFldEffScript_DestroyDeoxysRock[];
extern const u8 gFldEffScript_DoubleExclMarkIcon[];
extern const u8 gFldEffScript_Dust[];
extern const u8 gFldEffScript_ExclamationMarkIcon[];
extern const u8 gFldEffScript_FeetInFlowingWater[];
extern const u8 gFldEffScript_FieldMoveShowMon[];
extern const u8 gFldEffScript_FieldMoveShowMonInit[];
extern const u8 gFldEffScript_FlyIn[];
extern const u8 gFldEffScript_FlyOut[];
extern const u8 gFldEffScript_HallOfFameRecord[];
extern const u8 gFldEffScript_HotSpringsWater[];
extern const u8 gFldEffScript_JumpBigSplash[];
extern const u8 gFldEffScript_JumpLongGrass[];
extern const u8 gFldEffScript_JumpSmallSplash[];
extern const u8 gFldEffScript_JumpTallGrass[];
extern const u8 gFldEffScript_LavaridgeGymWarp[];
extern const u8 gFldEffScript_LongGrass[];
extern const u8 gFldEffScript_MountainDisguise[];
extern const u8 gFldEffScript_MoveDeoxysRock[];
extern const u8 gFldEffScript_Nop47[];
extern const u8 gFldEffScript_Nop48[];
extern const u8 gFldEffScript_NpcflyOut[];
extern const u8 gFldEffScript_PcturnOn[];
extern const u8 gFldEffScript_PhotoFlash[];
extern const u8 gFldEffScript_Pokeball[];
extern const u8 gFldEffScript_PokecenterHeal[];
extern const u8 gFldEffScript_PopOutOfAsh[];
extern const u8 gFldEffScript_QuestionMarkIcon[];
extern const u8 gFldEffScript_Ripple[];
extern const u8 gFldEffScript_SandDisguise[];
extern const u8 gFldEffScript_SandFootprints[];
extern const u8 gFldEffScript_SandPile[];
extern const u8 gFldEffScript_SandPillar[];
extern const u8 gFldEffScript_SecretPowerCave[];
extern const u8 gFldEffScript_SecretPowerShrub[];
extern const u8 gFldEffScript_SecretPowerTree[];
extern const u8 gFldEffScript_Shadow[];
extern const u8 gFldEffScript_ShortGrass[];
extern const u8 gFldEffScript_SmileyFaceIcon[];
extern const u8 gFldEffScript_Sparkle[];
extern const u8 gFldEffScript_Splash[];
extern const u8 gFldEffScript_SurfBlob[];
extern const u8 gFldEffScript_SweetScent[];
extern const u8 gFldEffScript_TallGrass[];
extern const u8 gFldEffScript_TreeDisguise[];
extern const u8 gFldEffScript_UnusedGrass[];
extern const u8 gFldEffScript_UnusedGrass2[];
extern const u8 gFldEffScript_UnusedSand[];
extern const u8 gFldEffScript_UnusedWaterSurfacing[];
extern const u8 gFldEffScript_UseCutOnGrass[];
extern const u8 gFldEffScript_UseCutOnTree[];
extern const u8 gFldEffScript_UseDig[];
extern const u8 gFldEffScript_UseDive[];
extern const u8 gFldEffScript_UseFlyAncientTomb[];
extern const u8 gFldEffScript_UseRockSmash[];
extern const u8 gFldEffScript_UseSecretPowerCave[];
extern const u8 gFldEffScript_UseSecretPowerShrub[];
extern const u8 gFldEffScript_UseSecretPowerTree[];
extern const u8 gFldEffScript_UseStrength[];
extern const u8 gFldEffScript_UseSurf[];
extern const u8 gFldEffScript_UseTeleport[];
extern const u8 gFldEffScript_UseVsSeeker[];
extern const u8 gFldEffScript_UseWaterfall[];
extern const u8 gFldEffScript_XIcon[];
extern u8 gFldEffPalette_CutGrass[];
extern u8 gSpritePalette_Ash[];
extern u8 gSpritePalette_GeneralFieldEffect0[];
extern u8 gSpritePalette_GeneralFieldEffect1[];
extern u8 gSpritePalette_HofMonitor[];
extern u8 gSpritePalette_Pokeball[];
extern u8 gSpritePalette_PokeballGlow[];
extern u8 gSpritePalette_SmallSparkle[];
extern void FldEff_Ash(void);
extern void FldEff_BerryTreeGrowthSparkle(void);
extern void FldEff_BikeTireTracks(void);
extern void FldEff_Bubbles(void);
extern void FldEff_CutGrass(void);
extern void FldEff_DeepSandFootprints(void);
extern void FldEff_DestroyDeoxysRock(void);
extern void FldEff_DoubleExclMarkIcon(void);
extern void FldEff_Dust(void);
extern void FldEff_ExclamationMarkIcon1(void);
extern void FldEff_FeetInFlowingWater(void);
extern void FldEff_FieldMoveShowMon(void);
extern void FldEff_FieldMoveShowMonInit(void);
extern void FldEff_FlyIn(void);
extern void FldEff_FlyOut(void);
extern void FldEff_HallOfFameRecord(void);
extern void FldEff_HotSpringsWater(void);
extern void FldEff_JumpBigSplash(void);
extern void FldEff_JumpLongGrass(void);
extern void FldEff_JumpSmallSplash(void);
extern void FldEff_JumpTallGrass(void);
extern void FldEff_LavaridgeGymWarp(void);
extern void FldEff_LongGrass(void);
extern void FldEff_MoveDeoxysRock(void);
extern void FldEff_NpcFlyOut(void);
extern void FldEff_PhotoFlash(void);
extern void FldEff_PokeballTrail(void);
extern void FldEff_PokecenterHeal(void);
extern void FldEff_PopOutOfAsh(void);
extern void FldEff_QuestionMarkIcon(void);
extern void FldEff_Ripple(void);
extern void FldEff_SandFootprints(void);
extern void FldEff_SandPile(void);
extern void FldEff_Shadow(void);
extern void FldEff_ShortGrass(void);
extern void FldEff_SmileyFaceIcon(void);
extern void FldEff_Sparkle(void);
extern void FldEff_Splash(void);
extern void FldEff_SurfBlob(void);
extern void FldEff_SweetScent(void);
extern void FldEff_TallGrass(void);
extern void FldEff_UnusedGrass(void);
extern void FldEff_UnusedGrass2(void);
extern void FldEff_UnusedSand(void);
extern void FldEff_UnusedWaterSurfacing(void);
extern void FldEff_UseCutOnGrass(void);
extern void FldEff_UseCutOnTree(void);
extern void FldEff_UseDig(void);
extern void FldEff_UseDive(void);
extern void FldEff_UseRockSmash(void);
extern void FldEff_UseStrength(void);
extern void FldEff_UseSurf(void);
extern void FldEff_UseTeleport(void);
extern void FldEff_UseVsSeeker(void);
extern void FldEff_UseWaterfall(void);
extern void FldEff_XIcon(void);
extern void ShowMountainDisguiseFieldEffect(void);
extern void ShowSandDisguiseFieldEffect(void);
extern void ShowTreeDisguiseFieldEffect(void);

const u8 *const gFieldEffectScriptPointers[] = {
    ((const void *)(uintptr_t)gFldEffScript_ExclamationMarkIcon),
    ((const void *)(uintptr_t)gFldEffScript_UseCutOnGrass),
    ((const void *)(uintptr_t)gFldEffScript_UseCutOnTree),
    ((const void *)(uintptr_t)gFldEffScript_Shadow),
    ((const void *)(uintptr_t)gFldEffScript_TallGrass),
    ((const void *)(uintptr_t)gFldEffScript_Ripple),
    ((const void *)(uintptr_t)gFldEffScript_FieldMoveShowMon),
    ((const void *)(uintptr_t)gFldEffScript_Ash),
    ((const void *)(uintptr_t)gFldEffScript_SurfBlob),
    ((const void *)(uintptr_t)gFldEffScript_UseSurf),
    ((const void *)(uintptr_t)gFldEffScript_Dust),
    ((const void *)(uintptr_t)gFldEffScript_UseSecretPowerCave),
    ((const void *)(uintptr_t)gFldEffScript_JumpTallGrass),
    ((const void *)(uintptr_t)gFldEffScript_SandFootprints),
    ((const void *)(uintptr_t)gFldEffScript_JumpBigSplash),
    ((const void *)(uintptr_t)gFldEffScript_Splash),
    ((const void *)(uintptr_t)gFldEffScript_JumpSmallSplash),
    ((const void *)(uintptr_t)gFldEffScript_LongGrass),
    ((const void *)(uintptr_t)gFldEffScript_JumpLongGrass),
    ((const void *)(uintptr_t)gFldEffScript_UnusedGrass),
    ((const void *)(uintptr_t)gFldEffScript_UnusedGrass2),
    ((const void *)(uintptr_t)gFldEffScript_UnusedSand),
    ((const void *)(uintptr_t)gFldEffScript_UnusedWaterSurfacing),
    ((const void *)(uintptr_t)gFldEffScript_BerryTreeGrowthSparkle),
    ((const void *)(uintptr_t)gFldEffScript_DeepSandFootprints),
    ((const void *)(uintptr_t)gFldEffScript_PokecenterHeal),
    ((const void *)(uintptr_t)gFldEffScript_UseSecretPowerTree),
    ((const void *)(uintptr_t)gFldEffScript_UseSecretPowerShrub),
    ((const void *)(uintptr_t)gFldEffScript_TreeDisguise),
    ((const void *)(uintptr_t)gFldEffScript_MountainDisguise),
    ((const void *)(uintptr_t)gFldEffScript_NpcflyOut),
    ((const void *)(uintptr_t)gFldEffScript_FlyOut),
    ((const void *)(uintptr_t)gFldEffScript_FlyIn),
    ((const void *)(uintptr_t)gFldEffScript_QuestionMarkIcon),
    ((const void *)(uintptr_t)gFldEffScript_FeetInFlowingWater),
    ((const void *)(uintptr_t)gFldEffScript_BikeTireTracks),
    ((const void *)(uintptr_t)gFldEffScript_SandDisguise),
    ((const void *)(uintptr_t)gFldEffScript_UseRockSmash),
    ((const void *)(uintptr_t)gFldEffScript_UseDig),
    ((const void *)(uintptr_t)gFldEffScript_SandPile),
    ((const void *)(uintptr_t)gFldEffScript_UseStrength),
    ((const void *)(uintptr_t)gFldEffScript_ShortGrass),
    ((const void *)(uintptr_t)gFldEffScript_HotSpringsWater),
    ((const void *)(uintptr_t)gFldEffScript_UseWaterfall),
    ((const void *)(uintptr_t)gFldEffScript_UseDive),
    ((const void *)(uintptr_t)gFldEffScript_Pokeball),
    ((const void *)(uintptr_t)gFldEffScript_XIcon),
    ((const void *)(uintptr_t)gFldEffScript_Nop47),
    ((const void *)(uintptr_t)gFldEffScript_Nop48),
    ((const void *)(uintptr_t)gFldEffScript_PopOutOfAsh),
    ((const void *)(uintptr_t)gFldEffScript_LavaridgeGymWarp),
    ((const void *)(uintptr_t)gFldEffScript_SweetScent),
    ((const void *)(uintptr_t)gFldEffScript_SandPillar),
    ((const void *)(uintptr_t)gFldEffScript_Bubbles),
    ((const void *)(uintptr_t)gFldEffScript_Sparkle),
    ((const void *)(uintptr_t)gFldEffScript_SecretPowerCave),
    ((const void *)(uintptr_t)gFldEffScript_SecretPowerTree),
    ((const void *)(uintptr_t)gFldEffScript_SecretPowerShrub),
    ((const void *)(uintptr_t)gFldEffScript_CutGrass),
    ((const void *)(uintptr_t)gFldEffScript_FieldMoveShowMonInit),
    ((const void *)(uintptr_t)gFldEffScript_UseFlyAncientTomb),
    ((const void *)(uintptr_t)gFldEffScript_PcturnOn),
    ((const void *)(uintptr_t)gFldEffScript_HallOfFameRecord),
    ((const void *)(uintptr_t)gFldEffScript_UseTeleport),
    ((const void *)(uintptr_t)gFldEffScript_SmileyFaceIcon),
    ((const void *)(uintptr_t)gFldEffScript_UseVsSeeker),
    ((const void *)(uintptr_t)gFldEffScript_DoubleExclMarkIcon),
    ((const void *)(uintptr_t)gFldEffScript_MoveDeoxysRock),
    ((const void *)(uintptr_t)gFldEffScript_DestroyDeoxysRock),
    ((const void *)(uintptr_t)gFldEffScript_PhotoFlash),
};

const u8 gFldEffScript_ExclamationMarkIcon[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 0u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 0u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 0u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 0u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseCutOnGrass[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 1u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 1u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 1u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 1u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseCutOnTree[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 2u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 2u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 2u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 2u)),
    ((u8)(4)),
};

const u8 gFldEffScript_Shadow[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 3u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 3u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 3u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 3u)),
    ((u8)(4)),
};

const u8 gFldEffScript_TallGrass[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 5u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 5u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 5u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 5u)),
    ((u8)(4)),
};

const u8 gFldEffScript_Ripple[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 6u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 6u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 6u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 6u)),
    ((u8)(4)),
};

const u8 gFldEffScript_FieldMoveShowMon[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 7u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 7u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 7u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 7u)),
    ((u8)(4)),
};

const u8 gFldEffScript_Ash[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 8u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 8u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 8u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 8u)),
    ((u8)(4)),
};

const u8 gFldEffScript_SurfBlob[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 9u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 9u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 9u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 9u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseSurf[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 10u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 10u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 10u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 10u)),
    ((u8)(4)),
};

const u8 gFldEffScript_Dust[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 12u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 12u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 12u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 12u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseSecretPowerCave[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_JumpTallGrass[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 13u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 13u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 13u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 13u)),
    ((u8)(4)),
};

const u8 gFldEffScript_SandFootprints[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 14u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 14u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 14u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 14u)),
    ((u8)(4)),
};

const u8 gFldEffScript_JumpBigSplash[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 15u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 15u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 15u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 15u)),
    ((u8)(4)),
};

const u8 gFldEffScript_Splash[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 16u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 16u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 16u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 16u)),
    ((u8)(4)),
};

const u8 gFldEffScript_JumpSmallSplash[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 17u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 17u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 17u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 17u)),
    ((u8)(4)),
};

const u8 gFldEffScript_LongGrass[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 18u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 18u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 18u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 18u)),
    ((u8)(4)),
};

const u8 gFldEffScript_JumpLongGrass[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 19u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 19u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 19u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 19u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UnusedGrass[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 20u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 20u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 20u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 20u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UnusedGrass2[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 21u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 21u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 21u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 21u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UnusedSand[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 22u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 22u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 22u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 22u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UnusedWaterSurfacing[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 23u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 23u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 23u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 23u)),
    ((u8)(4)),
};

const u8 gFldEffScript_BerryTreeGrowthSparkle[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 24u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 24u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 24u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 24u)),
    ((u8)(4)),
};

const u8 gFldEffScript_DeepSandFootprints[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 25u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 25u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 25u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 25u)),
    ((u8)(4)),
};

const u8 gFldEffScript_PokecenterHeal[] __attribute__((aligned(4))) = {
    ((u8)(1)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 26u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 26u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 26u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 26u)),
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 27u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 27u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 27u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 27u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseSecretPowerTree[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_UseSecretPowerShrub[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_TreeDisguise[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 28u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 28u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 28u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 28u)),
    ((u8)(4)),
};

const u8 gFldEffScript_MountainDisguise[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 29u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 29u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 29u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 29u)),
    ((u8)(4)),
};

const u8 gFldEffScript_NpcflyOut[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 30u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 30u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 30u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 30u)),
    ((u8)(4)),
};

const u8 gFldEffScript_FlyOut[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 31u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 31u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 31u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 31u)),
    ((u8)(4)),
};

const u8 gFldEffScript_FlyIn[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 32u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 32u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 32u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 32u)),
    ((u8)(4)),
};

const u8 gFldEffScript_QuestionMarkIcon[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 33u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 33u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 33u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 33u)),
    ((u8)(4)),
};

const u8 gFldEffScript_FeetInFlowingWater[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 34u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 34u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 34u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 34u)),
    ((u8)(4)),
};

const u8 gFldEffScript_BikeTireTracks[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 35u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 35u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 35u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 35u)),
    ((u8)(4)),
};

const u8 gFldEffScript_SandDisguise[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 36u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 36u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 36u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 36u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseRockSmash[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 37u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 37u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 37u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 37u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseStrength[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 38u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 38u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 38u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 38u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseDig[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 39u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 39u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 39u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 39u)),
    ((u8)(4)),
};

const u8 gFldEffScript_SandPile[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 40u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 40u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 40u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 40u)),
    ((u8)(4)),
};

const u8 gFldEffScript_ShortGrass[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 41u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 41u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 41u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 41u)),
    ((u8)(4)),
};

const u8 gFldEffScript_HotSpringsWater[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 4u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 42u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 42u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 42u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 42u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseWaterfall[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 43u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 43u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 43u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 43u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseDive[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 44u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 44u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 44u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 44u)),
    ((u8)(4)),
};

const u8 gFldEffScript_Pokeball[] __attribute__((aligned(4))) = {
    ((u8)(2)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 45u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 45u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 45u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 45u)),
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 46u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 46u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 46u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 46u)),
    ((u8)(4)),
};

const u8 gFldEffScript_XIcon[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 47u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 47u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 47u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 47u)),
    ((u8)(4)),
};

const u8 gFldEffScript_Nop47[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_Nop48[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_PopOutOfAsh[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 48u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 48u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 48u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 48u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 49u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 49u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 49u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 49u)),
    ((u8)(4)),
};

const u8 gFldEffScript_LavaridgeGymWarp[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 48u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 48u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 48u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 48u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 50u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 50u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 50u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 50u)),
    ((u8)(4)),
};

const u8 gFldEffScript_SweetScent[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 51u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 51u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 51u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 51u)),
    ((u8)(4)),
};

const u8 gFldEffScript_SandPillar[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_Bubbles[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 11u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 52u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 52u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 52u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 52u)),
    ((u8)(4)),
};

const u8 gFldEffScript_Sparkle[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 53u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 53u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 53u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 53u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 54u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 54u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 54u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 54u)),
    ((u8)(4)),
};

const u8 gFldEffScript_SecretPowerCave[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_SecretPowerTree[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_SecretPowerShrub[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_CutGrass[] __attribute__((aligned(4))) = {
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 55u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 55u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 55u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 55u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 56u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 56u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 56u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 56u)),
    ((u8)(4)),
};

const u8 gFldEffScript_FieldMoveShowMonInit[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 57u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 57u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 57u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 57u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseFlyAncientTomb[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_PcturnOn[] __attribute__((aligned(4))) = {
    ((u8)(4)),
};

const u8 gFldEffScript_HallOfFameRecord[] __attribute__((aligned(4))) = {
    ((u8)(1)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 26u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 26u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 26u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 26u)),
    ((u8)(7)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 58u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 58u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 58u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 58u)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 59u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 59u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 59u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 59u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseTeleport[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 60u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 60u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 60u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 60u)),
    ((u8)(4)),
};

const u8 gFldEffScript_SmileyFaceIcon[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 61u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 61u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 61u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 61u)),
    ((u8)(4)),
};

const u8 gFldEffScript_UseVsSeeker[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 62u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 62u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 62u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 62u)),
    ((u8)(4)),
};

const u8 gFldEffScript_DoubleExclMarkIcon[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 63u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 63u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 63u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 63u)),
    ((u8)(4)),
};

const u8 gFldEffScript_MoveDeoxysRock[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 64u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 64u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 64u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 64u)),
    ((u8)(4)),
};

const u8 gFldEffScript_DestroyDeoxysRock[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 65u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 65u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 65u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 65u)),
    ((u8)(4)),
};

const u8 gFldEffScript_PhotoFlash[] __attribute__((aligned(4))) = {
    ((u8)(3)),
    U32_B0((FIRERED_PORTABLE_PTR_TOKEN_BASE + 66u)),
    U32_B1((FIRERED_PORTABLE_PTR_TOKEN_BASE + 66u)),
    U32_B2((FIRERED_PORTABLE_PTR_TOKEN_BASE + 66u)),
    U32_B3((FIRERED_PORTABLE_PTR_TOKEN_BASE + 66u)),
    ((u8)(4)),
};

const void *const gFireredPortableFieldEffectScriptPtrs[] = {
    ((const void *)(uintptr_t)&FldEff_ExclamationMarkIcon1),
    ((const void *)(uintptr_t)&FldEff_UseCutOnGrass),
    ((const void *)(uintptr_t)&FldEff_UseCutOnTree),
    ((const void *)(uintptr_t)&FldEff_Shadow),
    ((const void *)(uintptr_t)&gSpritePalette_GeneralFieldEffect1),
    ((const void *)(uintptr_t)&FldEff_TallGrass),
    ((const void *)(uintptr_t)&FldEff_Ripple),
    ((const void *)(uintptr_t)&FldEff_FieldMoveShowMon),
    ((const void *)(uintptr_t)&FldEff_Ash),
    ((const void *)(uintptr_t)&FldEff_SurfBlob),
    ((const void *)(uintptr_t)&FldEff_UseSurf),
    ((const void *)(uintptr_t)&gSpritePalette_GeneralFieldEffect0),
    ((const void *)(uintptr_t)&FldEff_Dust),
    ((const void *)(uintptr_t)&FldEff_JumpTallGrass),
    ((const void *)(uintptr_t)&FldEff_SandFootprints),
    ((const void *)(uintptr_t)&FldEff_JumpBigSplash),
    ((const void *)(uintptr_t)&FldEff_Splash),
    ((const void *)(uintptr_t)&FldEff_JumpSmallSplash),
    ((const void *)(uintptr_t)&FldEff_LongGrass),
    ((const void *)(uintptr_t)&FldEff_JumpLongGrass),
    ((const void *)(uintptr_t)&FldEff_UnusedGrass),
    ((const void *)(uintptr_t)&FldEff_UnusedGrass2),
    ((const void *)(uintptr_t)&FldEff_UnusedSand),
    ((const void *)(uintptr_t)&FldEff_UnusedWaterSurfacing),
    ((const void *)(uintptr_t)&FldEff_BerryTreeGrowthSparkle),
    ((const void *)(uintptr_t)&FldEff_DeepSandFootprints),
    ((const void *)(uintptr_t)&gSpritePalette_PokeballGlow),
    ((const void *)(uintptr_t)&FldEff_PokecenterHeal),
    ((const void *)(uintptr_t)&ShowTreeDisguiseFieldEffect),
    ((const void *)(uintptr_t)&ShowMountainDisguiseFieldEffect),
    ((const void *)(uintptr_t)&FldEff_NpcFlyOut),
    ((const void *)(uintptr_t)&FldEff_FlyOut),
    ((const void *)(uintptr_t)&FldEff_FlyIn),
    ((const void *)(uintptr_t)&FldEff_QuestionMarkIcon),
    ((const void *)(uintptr_t)&FldEff_FeetInFlowingWater),
    ((const void *)(uintptr_t)&FldEff_BikeTireTracks),
    ((const void *)(uintptr_t)&ShowSandDisguiseFieldEffect),
    ((const void *)(uintptr_t)&FldEff_UseRockSmash),
    ((const void *)(uintptr_t)&FldEff_UseStrength),
    ((const void *)(uintptr_t)&FldEff_UseDig),
    ((const void *)(uintptr_t)&FldEff_SandPile),
    ((const void *)(uintptr_t)&FldEff_ShortGrass),
    ((const void *)(uintptr_t)&FldEff_HotSpringsWater),
    ((const void *)(uintptr_t)&FldEff_UseWaterfall),
    ((const void *)(uintptr_t)&FldEff_UseDive),
    ((const void *)(uintptr_t)&gSpritePalette_Pokeball),
    ((const void *)(uintptr_t)&FldEff_PokeballTrail),
    ((const void *)(uintptr_t)&FldEff_XIcon),
    ((const void *)(uintptr_t)&gSpritePalette_Ash),
    ((const void *)(uintptr_t)&FldEff_PopOutOfAsh),
    ((const void *)(uintptr_t)&FldEff_LavaridgeGymWarp),
    ((const void *)(uintptr_t)&FldEff_SweetScent),
    ((const void *)(uintptr_t)&FldEff_Bubbles),
    ((const void *)(uintptr_t)&gSpritePalette_SmallSparkle),
    ((const void *)(uintptr_t)&FldEff_Sparkle),
    ((const void *)(uintptr_t)&gFldEffPalette_CutGrass),
    ((const void *)(uintptr_t)&FldEff_CutGrass),
    ((const void *)(uintptr_t)&FldEff_FieldMoveShowMonInit),
    ((const void *)(uintptr_t)&gSpritePalette_HofMonitor),
    ((const void *)(uintptr_t)&FldEff_HallOfFameRecord),
    ((const void *)(uintptr_t)&FldEff_UseTeleport),
    ((const void *)(uintptr_t)&FldEff_SmileyFaceIcon),
    ((const void *)(uintptr_t)&FldEff_UseVsSeeker),
    ((const void *)(uintptr_t)&FldEff_DoubleExclMarkIcon),
    ((const void *)(uintptr_t)&FldEff_MoveDeoxysRock),
    ((const void *)(uintptr_t)&FldEff_DestroyDeoxysRock),
    ((const void *)(uintptr_t)&FldEff_PhotoFlash),
};
const u32 gFireredPortableFieldEffectScriptPtrCount = (u32)(sizeof(gFireredPortableFieldEffectScriptPtrs) / sizeof(gFireredPortableFieldEffectScriptPtrs[0]));

