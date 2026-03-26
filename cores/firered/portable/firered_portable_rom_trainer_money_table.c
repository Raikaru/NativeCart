#include "global.h"

#include "portable/firered_portable_rom_trainer_money_table.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_trainer_money_table_refresh_after_rom_load(void)
{
}

#else

#include "battle_main.h"
#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

#define FIRERED_TRAINER_MONEY_MAX_ROWS 256u

static const FireredRomU32TableProfileRow s_trainer_money_profile_rows[] = {
    { 0u, "__firered_builtin_trainer_money_profile_none__", 0u },
};

static struct TrainerMoney s_trainer_money_rom[FIRERED_TRAINER_MONEY_MAX_ROWS];
static u8 s_trainer_money_rom_active;

const struct TrainerMoney *gTrainerMoneyTableActive;

static void sync_active_ptr(void)
{
    gTrainerMoneyTableActive = s_trainer_money_rom_active ? (const struct TrainerMoney *)s_trainer_money_rom : NULL;
}

static bool8 trainer_money_rom_valid(const struct TrainerMoney *rows, u32 row_count)
{
    u32 i;

    if (row_count < 2u)
        return FALSE;

    for (i = 0u; i < row_count; i++)
    {
        if (rows[i].classId == 0xFFu)
        {
            if (i != row_count - 1u)
                return FALSE;
            return (bool8)(rows[i].value != 0u);
        }
        if (rows[i].value == 0u)
            return FALSE;
    }
    return FALSE;
}

void firered_portable_rom_trainer_money_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need_bytes;
    u32 max_pairs;
    u32 i;

    s_trainer_money_rom_active = 0;
    sync_active_ptr();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_TRAINER_MONEY_TABLE_OFF",
            s_trainer_money_profile_rows, NELEMS(s_trainer_money_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    max_pairs = (u32)sizeof(s_trainer_money_rom) / 2u;
    need_bytes = (size_t)max_pairs * sizeof(struct TrainerMoney);
    if (table_off > rom_size || need_bytes > rom_size - table_off)
        return;

    memcpy(s_trainer_money_rom, rom + table_off, need_bytes);

    for (i = 0u; i < max_pairs; i++)
    {
        if (s_trainer_money_rom[i].classId == 0xFFu)
            break;
    }

    if (i >= max_pairs)
        return;

    need_bytes = (size_t)(i + 1u) * sizeof(struct TrainerMoney);

    if (!trainer_money_rom_valid(s_trainer_money_rom, (u32)(need_bytes / sizeof(struct TrainerMoney))))
        return;

    /* Zero pad past terminator so no accidental reads past end. */
    memset((u8 *)s_trainer_money_rom + need_bytes, 0, sizeof(s_trainer_money_rom) - need_bytes);

    s_trainer_money_rom_active = 1;
    sync_active_ptr();
}

#endif /* PORTABLE */
