/* Host ROM pack buffers: engine_backend_init runs this before AgbMain InitHeap. */
#define FIRERED_HOST_LIBC_MALLOC 1

#include "global.h"

#include "portable/firered_portable_rom_battle_tower_mon_templates.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_battle_tower_mon_templates_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "battle_tower.h"
#include "constants/battle_tower.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "engine_internal.h"

#define FIRERED_BTTP_PACK_MAGIC 0x42545450u /* 'BTTP' LE */
#define FIRERED_BTTP_PACK_VERSION 1u
#define FIRERED_BTTP_HEADER_BYTES 24u

#define FIRERED_BT50_ENTRY_COUNT  ((size_t)BATTLE_TOWER_LEVEL50_MON_COUNT)
#define FIRERED_BT100_ENTRY_COUNT ((size_t)BATTLE_TOWER_LEVEL100_MON_COUNT)

const struct BattleTowerPokemonTemplate *gBattleTowerLevel50MonsActive;
const struct BattleTowerPokemonTemplate *gBattleTowerLevel100MonsActive;

static const FireredRomU32TableProfileRow s_bt_mon_templates_profile_rows[] = {
    { 0u, "__firered_builtin_battle_tower_mon_templates_profile_none__", 0u },
};

static struct BattleTowerPokemonTemplate *s_pool50;
static struct BattleTowerPokemonTemplate *s_pool100;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void bt_mon_templates_sync_active_ptr(void)
{
    gBattleTowerLevel50MonsActive = s_pool50;
    gBattleTowerLevel100MonsActive = s_pool100;
}

static int bt_template_row_semantic_ok(const struct BattleTowerPokemonTemplate *r)
{
    unsigned j;

    if (r->species == SPECIES_NONE || r->species >= NUM_SPECIES)
        return 0;
    if (r->heldItem > BATTLE_TOWER_ITEM_GANLON_BERRY)
        return 0;
    for (j = 0; j < 4u; j++)
    {
        if (r->moves[j] >= MOVES_COUNT)
            return 0;
    }
    if ((r->evSpread & ~0x3Fu) != 0)
        return 0;
    if (r->nature >= NUM_NATURES)
        return 0;
    return 1;
}

static int bt_pool_valid(const struct BattleTowerPokemonTemplate *p, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
    {
        if (!bt_template_row_semantic_ok(&p[i]))
            return 0;
    }
    return 1;
}

void firered_portable_rom_battle_tower_mon_templates_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;
    u32 magic;
    u32 version;
    u32 count50;
    u32 count100;
    u32 row_bytes;
    u32 reserved;
    size_t need;
    size_t body;
    struct BattleTowerPokemonTemplate *n50 = NULL;
    struct BattleTowerPokemonTemplate *n100 = NULL;

    free(s_pool50);
    free(s_pool100);
    s_pool50 = NULL;
    s_pool100 = NULL;
    bt_mon_templates_sync_active_ptr();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_BATTLE_TOWER_MON_TEMPLATES_PACK_OFF",
            s_bt_mon_templates_profile_rows, NELEMS(s_bt_mon_templates_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (pack_off > rom_size - FIRERED_BTTP_HEADER_BYTES)
        return;

    magic = read_le_u32(rom + pack_off);
    version = read_le_u32(rom + pack_off + 4u);
    count50 = read_le_u32(rom + pack_off + 8u);
    count100 = read_le_u32(rom + pack_off + 12u);
    row_bytes = read_le_u32(rom + pack_off + 16u);
    reserved = read_le_u32(rom + pack_off + 20u);

    if (magic != FIRERED_BTTP_PACK_MAGIC || version != FIRERED_BTTP_PACK_VERSION)
        return;
    if (reserved != 0u)
        return;
    if (count50 != (u32)FIRERED_BT50_ENTRY_COUNT || count100 != (u32)FIRERED_BT100_ENTRY_COUNT)
        return;
    if (row_bytes != (u32)sizeof(struct BattleTowerPokemonTemplate))
        return;

    body = (size_t)count50 * (size_t)row_bytes + (size_t)count100 * (size_t)row_bytes;
    need = FIRERED_BTTP_HEADER_BYTES + body;
    if (pack_off > rom_size - need)
        return;

    n50 = (struct BattleTowerPokemonTemplate *)malloc((size_t)count50 * sizeof(struct BattleTowerPokemonTemplate));
    n100 = (struct BattleTowerPokemonTemplate *)malloc((size_t)count100 * sizeof(struct BattleTowerPokemonTemplate));
    if (n50 == NULL || n100 == NULL)
    {
        free(n50);
        free(n100);
        return;
    }

    memcpy(n50, rom + pack_off + FIRERED_BTTP_HEADER_BYTES, (size_t)count50 * (size_t)row_bytes);
    memcpy(n100, rom + pack_off + FIRERED_BTTP_HEADER_BYTES + (size_t)count50 * (size_t)row_bytes,
        (size_t)count100 * (size_t)row_bytes);

    if (!bt_pool_valid(n50, (size_t)count50) || !bt_pool_valid(n100, (size_t)count100))
    {
        free(n50);
        free(n100);
        return;
    }

    s_pool50 = n50;
    s_pool100 = n100;
    bt_mon_templates_sync_active_ptr();
}

#endif /* PORTABLE */
