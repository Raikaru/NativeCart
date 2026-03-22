#include "global.h"
#include "../../../engine/core/engine_internal.h"

#include "portable/firered_portable_rom_battle_ai_fragment_prefix.h"
#include "portable/firered_portable_rom_event_script_pointer_overlay.h"
#include "portable/firered_portable_rom_map_object_event_script_pointer_overlay.h"
#include "portable/firered_portable_rom_battle_script_pointer_overlay.h"
#include "portable/firered_portable_rom_field_effect_pointer_overlay.h"
#include "portable/firered_portable_rom_battle_anim_pointer_overlay.h"

#include <string.h>

#define FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE 0x80000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT 0x81000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_FIELD_EFFECT 0x82000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI 0x83000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_ANIM 0x84000000u
#define FIRERED_PORTABLE_PTR_TOKEN_BASE_MAP_OBJECT 0x85000000u
/* Exclusive upper bound for portable-encoded pointer tokens (incl. map-object range). */
#define FIRERED_PORTABLE_PTR_TOKEN_ZONE_END 0x86000000u

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
extern const void *const gFireredPortableMapObjectEventScriptPtrs[];
extern const uint32_t gFireredPortableMapObjectEventScriptPtrCount;

static const void *firered_portable_fix_battle_ai_ptr(const void *ptr)
{
    const u8 *scriptPtr = ptr;

    if (scriptPtr != NULL && scriptPtr[0] == 0x83 && scriptPtr[1] < 0x80)
        return scriptPtr + 1;

    return ptr;
}

static size_t portable_strnlen_local(const char *s, size_t maxLen)
{
    size_t n = 0;

    while (n < maxLen && s[n] != '\0')
        n++;
    return n;
}

static u32 firered_portable_try_cstring_to_rom_address(const char *str)
{
    size_t len;
    const u8 *romBase;
    size_t off;

    if (str == NULL)
        return 0;

    len = portable_strnlen_local(str, 512);
    if (len == 0)
        return 0;

    romBase = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    for (off = 0; off + len < ENGINE_ROM_SIZE; off++)
    {
        if (memcmp(romBase + off, str, len) != 0)
            continue;
        if (romBase[off + len] != '\0')
            continue;
        return (u32)(ENGINE_ROM_ADDR + (uint32_t)off);
    }
    return 0;
}

static int firered_portable_ptr_in_region(uintptr_t p, uintptr_t base, size_t size)
{
    return p >= base && p < base + size;
}

u32 firered_portable_ptr_to_save_u32(const void *ptr)
{
    uintptr_t p;
    uint32_t i;

    if (ptr == NULL)
        return 0;

    p = (uintptr_t)ptr;

    /* Values already encoded as portable tokens (see resolve). */
    if (p <= (uintptr_t)0xFFFFFFFFu && p >= FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE
        && p < (uintptr_t)FIRERED_PORTABLE_PTR_TOKEN_ZONE_END)
        return (u32)p;

    /* Engine maps GBA bus regions at their canonical addresses — save the same u32 as retail. */
    if (firered_portable_ptr_in_region(p, ENGINE_EWRAM_ADDR, ENGINE_EWRAM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_IWRAM_ADDR, ENGINE_IWRAM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_IOREG_ADDR, ENGINE_IOREG_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_PALETTE_ADDR, ENGINE_PALETTE_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_VRAM_ADDR, ENGINE_VRAM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_OAM_ADDR, ENGINE_OAM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_ROM_ADDR, ENGINE_ROM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_SRAM_ADDR, ENGINE_SRAM_SIZE))
    {
        return (u32)p;
    }

    for (i = 0; i < gFireredPortableBattleAnimPtrCount; i++)
    {
        if (gFireredPortableBattleAnimPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_ANIM + i;
    }

    for (i = 0; i < gFireredPortableBattleAiPtrCount; i++)
    {
        const u8 *e = (const u8 *)gFireredPortableBattleAiPtrs[i];

        if ((const void *)e == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI + i;
        if (e != NULL && e[0] == 0x83 && e[1] < 0x80 && (const void *)(e + 1) == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI + i;
    }

    for (i = 0; i < gFireredPortableFieldEffectScriptPtrCount; i++)
    {
        if (gFireredPortableFieldEffectScriptPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_FIELD_EFFECT + i;
    }

    for (i = 0; i < gFireredPortableEventScriptPtrCount; i++)
    {
        if (gFireredPortableEventScriptPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT + i;
    }

    for (i = 0; i < gFireredPortableBattleScriptPtrCount; i++)
    {
        if (gFireredPortableBattleScriptPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE + i;
    }

    for (i = 0; i < gFireredPortableMapObjectEventScriptPtrCount; i++)
    {
        if (gFireredPortableMapObjectEventScriptPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_MAP_OBJECT + i;
    }

    /* Host .rodata (e.g. berry description strings): locate identical bytes in loaded cartridge ROM. */
    return firered_portable_try_cstring_to_rom_address((const char *)ptr);
}

u32 firered_portable_map_object_event_script_ptr_to_save_u32(const void *ptr)
{
    uintptr_t p;
    uint32_t i;

    if (ptr == NULL)
        return 0;

    p = (uintptr_t)ptr;

    if (p <= (uintptr_t)0xFFFFFFFFu && p >= FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE
        && p < (uintptr_t)FIRERED_PORTABLE_PTR_TOKEN_ZONE_END)
        return (u32)p;

    if (firered_portable_ptr_in_region(p, ENGINE_EWRAM_ADDR, ENGINE_EWRAM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_IWRAM_ADDR, ENGINE_IWRAM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_IOREG_ADDR, ENGINE_IOREG_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_PALETTE_ADDR, ENGINE_PALETTE_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_VRAM_ADDR, ENGINE_VRAM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_OAM_ADDR, ENGINE_OAM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_ROM_ADDR, ENGINE_ROM_SIZE)
        || firered_portable_ptr_in_region(p, ENGINE_SRAM_ADDR, ENGINE_SRAM_SIZE))
    {
        return (u32)p;
    }

    for (i = 0; i < gFireredPortableBattleAnimPtrCount; i++)
    {
        if (gFireredPortableBattleAnimPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_ANIM + i;
    }

    for (i = 0; i < gFireredPortableBattleAiPtrCount; i++)
    {
        const u8 *e = (const u8 *)gFireredPortableBattleAiPtrs[i];

        if ((const void *)e == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI + i;
        if (e != NULL && e[0] == 0x83 && e[1] < 0x80 && (const void *)(e + 1) == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI + i;
    }

    for (i = 0; i < gFireredPortableFieldEffectScriptPtrCount; i++)
    {
        if (gFireredPortableFieldEffectScriptPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_FIELD_EFFECT + i;
    }

    for (i = 0; i < gFireredPortableEventScriptPtrCount; i++)
    {
        if (gFireredPortableEventScriptPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT + i;
    }

    for (i = 0; i < gFireredPortableBattleScriptPtrCount; i++)
    {
        if (gFireredPortableBattleScriptPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE + i;
    }

    for (i = 0; i < gFireredPortableMapObjectEventScriptPtrCount; i++)
    {
        if (gFireredPortableMapObjectEventScriptPtrs[i] == ptr)
            return FIRERED_PORTABLE_PTR_TOKEN_BASE_MAP_OBJECT + i;
    }

    return 0;
}

const void *firered_portable_resolve_script_ptr(uint32_t value)
{
    if (value == 0)
        return NULL;

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_MAP_OBJECT)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_MAP_OBJECT;

        if (tokenIndex < gFireredPortableMapObjectEventScriptPtrCount)
        {
            const void *ptr;

            ptr = NULL;
            if (tokenIndex >= FIRERED_PORTABLE_MAP_OBJECT_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX)
                ptr = (const void *)firered_portable_rom_map_object_event_script_pointer_overlay_ptr(tokenIndex);
            if (ptr == NULL)
                ptr = gFireredPortableMapObjectEventScriptPtrs[tokenIndex];
            return ptr;
        }
        return NULL;
    }

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_ANIM)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_ANIM;
        if (tokenIndex < gFireredPortableBattleAnimPtrCount)
        {
            const void *ptr;

            ptr = NULL;
            if (tokenIndex >= FIRERED_PORTABLE_BATTLE_ANIM_ROM_OVERLAY_FIRST_INDEX)
                ptr = (const void *)firered_portable_rom_battle_anim_pointer_overlay_ptr(tokenIndex);
            if (ptr == NULL)
                ptr = gFireredPortableBattleAnimPtrs[tokenIndex];
            return ptr;
        }
        return NULL;
    }

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE_AI;
        if (tokenIndex < gFireredPortableBattleAiPtrCount)
        {
            const void *ptr;

            ptr = (const void *)firered_portable_rom_battle_ai_fragment_prefix_ptr(tokenIndex);
            if (ptr == NULL)
                ptr = gFireredPortableBattleAiPtrs[tokenIndex];
            return firered_portable_fix_battle_ai_ptr(ptr);
        }
        return NULL;
    }

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_FIELD_EFFECT)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_FIELD_EFFECT;
        if (tokenIndex < gFireredPortableFieldEffectScriptPtrCount)
        {
            const void *ptr;

            ptr = NULL;
            if (tokenIndex >= FIRERED_PORTABLE_FIELD_EFFECT_ROM_OVERLAY_FIRST_INDEX)
                ptr = (const void *)firered_portable_rom_field_effect_pointer_overlay_ptr(tokenIndex);
            if (ptr == NULL)
                ptr = gFireredPortableFieldEffectScriptPtrs[tokenIndex];
            return ptr;
        }
        return NULL;
    }

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT;
        if (tokenIndex < gFireredPortableEventScriptPtrCount)
        {
            const void *ptr;

            ptr = NULL;
            if (tokenIndex >= FIRERED_PORTABLE_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX)
                ptr = (const void *)firered_portable_rom_event_script_pointer_overlay_ptr(tokenIndex);
            if (ptr == NULL)
                ptr = gFireredPortableEventScriptPtrs[tokenIndex];
            return ptr;
        }
        return NULL;
    }

    if (value >= FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE)
    {
        uint32_t tokenIndex = value - FIRERED_PORTABLE_PTR_TOKEN_BASE_BATTLE;
        if (tokenIndex < gFireredPortableBattleScriptPtrCount)
        {
            const void *ptr;

            ptr = NULL;
            if (tokenIndex >= FIRERED_PORTABLE_BATTLE_SCRIPT_ROM_OVERLAY_FIRST_INDEX)
                ptr = (const void *)firered_portable_rom_battle_script_pointer_overlay_ptr(tokenIndex);
            if (ptr == NULL)
                ptr = gFireredPortableBattleScriptPtrs[tokenIndex];
            return ptr;
        }
        return NULL;
    }

    return (const void *)(uintptr_t)value;
}
