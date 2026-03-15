#include "global.h"
#include "../../../engine/core/engine_internal.h"

#define FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE 0x80000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT 0x81000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_FIELD_EFFECT 0x82000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI 0x83000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_ANIM 0x84000000u

extern const void *const gFireredPortableBattleScriptPtrs[];
extern const uint32_t gFireredPortableBattleScriptPtrCount;
extern const void *const gFireredPortableEventScriptPtrs[];
extern const uint32_t gFireredPortableEventScriptPtrCount;
extern const void *const gFireredPortableFieldEffectScriptPtrs[];
extern const uint32_t gFireredPortableFieldEffectScriptPtrCount;
extern const void *const gFireredPortableBattleAiPtrs[];
extern const uint32_t gFireredPortableBattleAiPtrCount;
extern const void *const gFireredPortableBattleAnimPtrs[];
extern const uint32_t gFireredPortableBattleAnimPtrCount;

static const void *firered_portable_fix_battle_ai_ptr(const void *ptr)
{
    const u8 *scriptPtr = ptr;

    if (scriptPtr != NULL && scriptPtr[0] == 0x83 && scriptPtr[1] < 0x80)
        return scriptPtr + 1;

    return ptr;
}

const void *firered_portable_resolve_script_ptr(uint32_t value)
{
    if (value == 0)
        return NULL;

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_ANIM)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_ANIM;
        if (tokenIndex < gFireredPortableBattleAnimPtrCount)
            return gFireredPortableBattleAnimPtrs[tokenIndex];
        return NULL;
    }

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI;
        if (tokenIndex < gFireredPortableBattleAiPtrCount)
            return firered_portable_fix_battle_ai_ptr(gFireredPortableBattleAiPtrs[tokenIndex]);
        return NULL;
    }

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_FIELD_EFFECT)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_FIELD_EFFECT;
        if (tokenIndex < gFireredPortableFieldEffectScriptPtrCount)
            return gFireredPortableFieldEffectScriptPtrs[tokenIndex];
        return NULL;
    }

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT;
        if (tokenIndex < gFireredPortableEventScriptPtrCount)
            return gFireredPortableEventScriptPtrs[tokenIndex];
        return NULL;
    }

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE;
        if (tokenIndex < gFireredPortableBattleScriptPtrCount)
            return gFireredPortableBattleScriptPtrs[tokenIndex];
        return NULL;
    }

    return (const void *)(uintptr_t)value;
}
