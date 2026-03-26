#include "global.h"

#include "portable/firered_portable_rom_experience_tables.h"

#include "constants/pokemon.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_experience_tables_refresh_after_rom_load(void)
{
}

FireredExperienceTableProvenanceState firered_portable_rom_experience_tables_get_provenance_state(void)
{
    return FIRERED_EXPERIENCE_TABLE_PROVENANCE_UNCHANGED;
}

const char *firered_portable_rom_experience_tables_get_provenance_state_name(void)
{
    return "UNCHANGED";
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

/* Matches src/data/pokemon/experience_tables.h: six growth-rate rows. */
#define FIRERED_ROM_EXPERIENCE_GROWTH_ROW_COUNT 6u
#define FIRERED_ROM_EXPERIENCE_LEVEL_COLUMN_COUNT ((size_t)(MAX_LEVEL + 1))
#define FIRERED_ROM_EXPERIENCE_TABLE_U32_TOTAL (FIRERED_ROM_EXPERIENCE_GROWTH_ROW_COUNT * FIRERED_ROM_EXPERIENCE_LEVEL_COLUMN_COUNT)

static const FireredRomU32TableProfileRow s_experience_tables_profile_rows[] = {
    { 0u, "__firered_builtin_experience_tables_profile_none__", 0u },
};

static u32 s_experience_tables_rom[FIRERED_ROM_EXPERIENCE_TABLE_U32_TOTAL];
static u8 s_experience_tables_rom_active;
static FireredExperienceTableProvenanceState s_experience_tables_provenance;

extern const u32 gExperienceTables[][MAX_LEVEL + 1];

static void read_le_u32_block(const u8 *rom, size_t table_off, size_t word_count, u32 *out)
{
    size_t i;

    for (i = 0; i < word_count; i++)
    {
        size_t o = table_off + i * 4u;
        out[i] = (u32)rom[o] | ((u32)rom[o + 1u] << 8) | ((u32)rom[o + 2u] << 16) | ((u32)rom[o + 3u] << 24);
    }
}

static int experience_tables_rows_monotonic(const u32 *buf, size_t rows, size_t cols)
{
    size_t r, c;

    for (r = 0; r < rows; r++)
    {
        const u32 *row = buf + r * cols;
        for (c = 1; c < cols; c++)
        {
            if (row[c - 1u] > row[c])
                return 0;
        }
    }

    return 1;
}

void firered_portable_rom_experience_tables_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t need;
    size_t n;
    u32 candidate[FIRERED_ROM_EXPERIENCE_TABLE_U32_TOTAL];
    const u32 *compiled = &gExperienceTables[0][0];
    int changed;

    s_experience_tables_rom_active = 0;
    s_experience_tables_provenance = FIRERED_EXPERIENCE_TABLE_PROVENANCE_UNCHANGED;

    n = (size_t)FIRERED_ROM_EXPERIENCE_TABLE_U32_TOTAL;
    need = n * 4u;
    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_EXPERIENCE_TABLES_OFF",
            s_experience_tables_profile_rows, NELEMS(s_experience_tables_profile_rows), &table_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (table_off > rom_size || need > rom_size - table_off)
        return;

    read_le_u32_block(rom, table_off, n, candidate);
    changed = (memcmp(candidate, compiled, need) != 0);
    if (!changed)
        return;

    if (!experience_tables_rows_monotonic(candidate, (size_t)FIRERED_ROM_EXPERIENCE_GROWTH_ROW_COUNT,
            FIRERED_ROM_EXPERIENCE_LEVEL_COLUMN_COUNT))
    {
        s_experience_tables_provenance = FIRERED_EXPERIENCE_TABLE_PROVENANCE_CHANGED_BUT_FELL_BACK;
        return;
    }

    memcpy(s_experience_tables_rom, candidate, need);
    s_experience_tables_rom_active = 1;
    s_experience_tables_provenance = FIRERED_EXPERIENCE_TABLE_PROVENANCE_CHANGED_AND_BOUND;
}

u32 ExperienceTableGet(u8 growthRate, u8 level)
{
    if (s_experience_tables_rom_active && (u32)growthRate < FIRERED_ROM_EXPERIENCE_GROWTH_ROW_COUNT
        && (u32)level < FIRERED_ROM_EXPERIENCE_LEVEL_COLUMN_COUNT)
    {
        size_t idx = (size_t)growthRate * FIRERED_ROM_EXPERIENCE_LEVEL_COLUMN_COUNT + (size_t)level;
        return s_experience_tables_rom[idx];
    }

    return gExperienceTables[growthRate][level];
}

FireredExperienceTableProvenanceState firered_portable_rom_experience_tables_get_provenance_state(void)
{
    return s_experience_tables_provenance;
}

const char *firered_portable_rom_experience_tables_get_provenance_state_name(void)
{
    switch (s_experience_tables_provenance)
    {
    case FIRERED_EXPERIENCE_TABLE_PROVENANCE_CHANGED_AND_BOUND:
        return "CHANGED_AND_BOUND";
    case FIRERED_EXPERIENCE_TABLE_PROVENANCE_CHANGED_BUT_FELL_BACK:
        return "CHANGED_BUT_FELL_BACK";
    case FIRERED_EXPERIENCE_TABLE_PROVENANCE_UNCHANGED:
    default:
        return "UNCHANGED";
    }
}

#endif /* PORTABLE */
