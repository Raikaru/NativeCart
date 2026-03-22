#include "global.h"

#include "portable/firered_portable_rom_level_up_learnsets_family.h"

#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

/* Table + refresh are compile-time only outside PORTABLE; this TU is not used. */

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "engine_internal.h"
#include "pokemon.h"

#define FIRERED_LUL_PACK_MAGIC 0x314C554Cu /* 'LUL1' LE */
#define FIRERED_LUL_PACK_VERSION 1u
#define FIRERED_LUL_HEADER_U32S 6u
#define FIRERED_LUL_HEADER_BYTES (FIRERED_LUL_HEADER_U32S * 4u)
#define FIRERED_LUL_MAX_STEPS_PER_SPECIES 2048u

static const FireredRomU32TableProfileRow s_level_up_learnsets_profile_rows[] = {
    { 0u, "__firered_builtin_level_up_learnsets_profile_none__", 0u },
};

static u8 *s_blob;
static const u16 **s_row_ptrs;
static u8 s_level_up_learnsets_rom_active;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/* Half-open intervals [a0, a1ex) and [b0, b1ex). */
static bool8 ranges_overlap_ex(size_t a0, size_t a1ex, size_t b0, size_t b1ex)
{
    if (a1ex <= a0 || b1ex <= b0)
        return FALSE;
    if (a0 < b1ex && b0 < a1ex)
        return TRUE;
    return FALSE;
}

static void free_family(void)
{
    free(s_blob);
    free(s_row_ptrs);
    s_blob = NULL;
    s_row_ptrs = NULL;
    s_level_up_learnsets_rom_active = 0;
}

static bool8 validate_learnset_list(const u8 *blob, size_t blob_len, u32 rel_off)
{
    size_t pos;
    u32 steps;
    u16 w;
    u8 level;
    u16 move;

    if (blob_len < 2u || (rel_off % 2u) != 0u || rel_off >= blob_len)
        return FALSE;

    pos = (size_t)rel_off;
    for (steps = 0u; steps < FIRERED_LUL_MAX_STEPS_PER_SPECIES; steps++)
    {
        if (pos + 2u > blob_len)
            return FALSE;
        w = (u16)(blob[pos] | (blob[pos + 1u] << 8));
        pos += 2u;

        if (w == LEVEL_UP_END)
            return TRUE;

        level = (u8)((w & LEVEL_UP_MOVE_LV) >> 9);
        move = (u16)(w & LEVEL_UP_MOVE_ID);

        if (level < 1u || level > MAX_LEVEL)
            return FALSE;
        if (move >= MOVES_COUNT)
            return FALSE;
    }
    return FALSE;
}

static bool8 load_family(const u8 *rom, size_t rom_size, size_t pack_off)
{
    u32 magic;
    u32 version;
    u32 num_species_pack;
    u32 ptr_off;
    u32 blob_off;
    u32 blob_len;
    size_t ptr_size;
    size_t ptr_end;
    size_t blob_end;
    size_t sp;
    const u8 *pack;
    u8 *blob_copy;
    const u16 **rows;

    if (pack_off > rom_size || pack_off + FIRERED_LUL_HEADER_BYTES > rom_size)
        return FALSE;

    pack = rom + pack_off;
    magic = read_le_u32(pack);
    version = read_le_u32(pack + 4u);
    num_species_pack = read_le_u32(pack + 8u);
    ptr_off = read_le_u32(pack + 12u);
    blob_off = read_le_u32(pack + 16u);
    blob_len = read_le_u32(pack + 20u);

    if (magic != FIRERED_LUL_PACK_MAGIC || version != FIRERED_LUL_PACK_VERSION)
        return FALSE;
    if (num_species_pack != (u32)NUM_SPECIES)
        return FALSE;
    if (blob_len == 0u || (blob_len % 2u) != 0u)
        return FALSE;

    ptr_size = (size_t)NUM_SPECIES * sizeof(u32);
    ptr_end = (size_t)ptr_off + ptr_size;
    blob_end = (size_t)blob_off + (size_t)blob_len;

    if (ptr_off < FIRERED_LUL_HEADER_BYTES || blob_off < FIRERED_LUL_HEADER_BYTES)
        return FALSE;
    if (ptr_end < (size_t)ptr_off || blob_end < (size_t)blob_off)
        return FALSE;
    if (ranges_overlap_ex((size_t)ptr_off, ptr_end, (size_t)blob_off, blob_end))
        return FALSE;

    if (ptr_end > rom_size - pack_off || blob_end > rom_size - pack_off)
        return FALSE;

    blob_copy = (u8 *)malloc((size_t)blob_len);
    rows = (const u16 **)calloc((size_t)NUM_SPECIES, sizeof(const u16 *));
    if (blob_copy == NULL || rows == NULL)
    {
        free(blob_copy);
        free(rows);
        return FALSE;
    }

    memcpy(blob_copy, rom + pack_off + (size_t)blob_off, (size_t)blob_len);

    for (sp = 0; sp < (size_t)NUM_SPECIES; sp++)
    {
        u32 rel;
        const u8 *pt = rom + pack_off + (size_t)ptr_off + sp * sizeof(u32);

        rel = read_le_u32(pt);
        if ((rel % 2u) != 0u || (size_t)rel >= (size_t)blob_len)
            goto fail;
        if (!validate_learnset_list(blob_copy, (size_t)blob_len, rel))
            goto fail;
        rows[sp] = (const u16 *)(void *)(blob_copy + (size_t)rel);
    }

    free_family();
    s_blob = blob_copy;
    s_row_ptrs = rows;
    s_level_up_learnsets_rom_active = 1;
    return TRUE;

fail:
    free(blob_copy);
    free(rows);
    return FALSE;
}

void firered_portable_rom_level_up_learnsets_family_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;

    free_family();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_LEVEL_UP_LEARNSETS_PACK_OFF",
            s_level_up_learnsets_profile_rows, NELEMS(s_level_up_learnsets_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    (void)load_family(rom, rom_size, pack_off);
}

const u16 *const *FireredLevelUpLearnsetsTable(void)
{
    if (s_level_up_learnsets_rom_active && s_row_ptrs != NULL)
        return (const u16 *const *)s_row_ptrs;
    return gLevelUpLearnsets_Compiled;
}

#endif /* PORTABLE */
