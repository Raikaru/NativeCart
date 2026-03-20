#ifndef GUARD_GLOBAL_BERRY_H
#define GUARD_GLOBAL_BERRY_H

#define BERRY_NAME_LENGTH 6
#define BERRY_NAME_COUNT 7
#define BERRY_ITEM_EFFECT_COUNT 18

struct Berry
{
    const u8 name[BERRY_NAME_COUNT];
    u8 firmness;
    u16 size;
    u8 maxYield;
    u8 minYield;
    const u8 *description1;
    const u8 *description2;
    u8 stageDuration;
    u8 spicy;
    u8 dry;
    u8 sweet;
    u8 bitter;
    u8 sour;
    u8 smoothness;
};

// with no const fields

#ifdef PORTABLE
#include <stddef.h>
#include <stdint.h>
/* Save layout: ROM pointers must be 4 bytes like GBA (not host pointers). */
struct __attribute__((packed)) Berry2
{
    u8 name[BERRY_NAME_COUNT];
    u8 firmness;
    u16 size;
    u8 maxYield;
    u8 minYield;
    u32 description1;
    u32 description2;
    u8 stageDuration;
    u8 spicy;
    u8 dry;
    u8 sweet;
    u8 bitter;
    u8 sour;
    u8 smoothness;
    u8 paddingToGbaSize0x1C; /* GBA sizeof(struct Berry2) == 0x1C */
};
#define BERRY2_DESC1_AS_PTR(_b) ((u8 *)(uintptr_t)((_b)->description1))
#define BERRY2_DESC2_AS_PTR(_b) ((u8 *)(uintptr_t)((_b)->description2))
#else
struct Berry2
{
    u8 name[BERRY_NAME_COUNT];
    u8 firmness;
    u16 size;
    u8 maxYield;
    u8 minYield;
    u8 *description1;
    u8 *description2;
    u8 stageDuration;
    u8 spicy;
    u8 dry;
    u8 sweet;
    u8 bitter;
    u8 sour;
    u8 smoothness;
};
#endif

struct EnigmaBerry
{
    struct Berry2 berry;
    u8 itemEffect[BERRY_ITEM_EFFECT_COUNT];
    u8 holdEffect;
    u8 holdEffectParam;
    u32 checksum;
};

struct BattleEnigmaBerry
{
    /*0x00*/ u8 name[BERRY_NAME_COUNT];
    /*0x07*/ u8 holdEffect;
    /*0x08*/ u8 itemEffect[BERRY_ITEM_EFFECT_COUNT];
    /*0x1A*/ u8 holdEffectParam;
};

struct BerryTree
{
    u8 berry;
    u8 stage:7;
    u8 growthSparkle:1;
    u16 minutesUntilNextStage;
    u8 berryYield;
    u8 regrowthCount:4;
    u8 watered1:1;
    u8 watered2:1;
    u8 watered3:1;
    u8 watered4:1;
};

#endif // GUARD_GLOBAL_BERRY_H
