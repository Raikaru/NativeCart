#include "global.h"

#include "portable/firered_portable_rom_type_effectiveness_table.h"

#include "battle_main.h"

#include "constants/pokemon.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_type_effectiveness_table_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

static const FireredRomU32TableProfileRow s_type_effectiveness_profile_rows[] = {
    { 0u, "__firered_builtin_type_effectiveness_profile_none__", 0u },
};

static u8 s_type_effectiveness_rom[FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT];
static u8 s_type_effectiveness_rom_active;

extern const u8 gTypeEffectiveness[FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT];

/* battle_main.h: PORTABLE TYPE_EFFECT_* macros read through this (fallback when NULL). */
const u8 *gTypeEffectivenessActivePtr;

static int type_effectiveness_buf_valid(const u8 *buf)
{
    size_t i;

    if (buf[FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT - 3u] != TYPE_ENDTABLE
        || buf[FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT - 2u] != TYPE_ENDTABLE)
        return 0;

    for (i = 0; i + 2u < FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT; i += 3u)
    {
        u8 a = buf[i];
        u8 d = buf[i + 1u];
        u8 m = buf[i + 2u];

        if (a == TYPE_ENDTABLE)
        {
            /* Terminator row must be the last triplet. */
            if (i != FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT - 3u)
                return 0;
            if (m != TYPE_MUL_NO_EFFECT && m != TYPE_MUL_NOT_EFFECTIVE && m != TYPE_MUL_NORMAL
                && m != TYPE_MUL_SUPER_EFFECTIVE)
                return 0;
            return 1;
        }

        if (m != TYPE_MUL_NO_EFFECT && m != TYPE_MUL_NOT_EFFECTIVE && m != TYPE_MUL_NORMAL
            && m != TYPE_MUL_SUPER_EFFECTIVE)
            return 0;

        if (a == TYPE_FORESIGHT)
        {
            if (d != TYPE_FORESIGHT)
                return 0;
        }
        else
        {
            if ((u32)a >= (u32)NUMBER_OF_MON_TYPES || (u32)d >= (u32)NUMBER_OF_MON_TYPES)
                return 0;
        }
    }

    return 0;
}

static void type_effectiveness_sync_active_ptr(void)
{
    gTypeEffectivenessActivePtr = s_type_effectiveness_rom_active ? s_type_effectiveness_rom : gTypeEffectiveness;
}

void firered_portable_rom_type_effectiveness_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;

    s_type_effectiveness_rom_active = 0;
    type_effectiveness_sync_active_ptr();

    need = FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_TYPE_EFFECTIVENESS_OFF",
            s_type_effectiveness_profile_rows, NELEMS(s_type_effectiveness_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    memcpy(s_type_effectiveness_rom, rom + table_off, FIRERED_TYPE_EFFECTIVENESS_BYTE_COUNT);
    if (!type_effectiveness_buf_valid(s_type_effectiveness_rom))
        return;

    s_type_effectiveness_rom_active = 1;
    type_effectiveness_sync_active_ptr();
}

#endif /* PORTABLE */
