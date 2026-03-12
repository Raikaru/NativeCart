#include "global.h"
#include "../../../engine/core/engine_internal.h"

#define FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT 0x81000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_FIELD_EFFECT 0x82000000u

extern const void *const gFireredPortableEventScriptPtrs[];
extern const uint32_t gFireredPortableEventScriptPtrCount;
extern const void *const gFireredPortableFieldEffectScriptPtrs[];
extern const uint32_t gFireredPortableFieldEffectScriptPtrCount;

const void *firered_portable_resolve_script_ptr(uint32_t value)
{
    if (value == 0)
        return NULL;

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

    return (const void *)(uintptr_t)value;
}
