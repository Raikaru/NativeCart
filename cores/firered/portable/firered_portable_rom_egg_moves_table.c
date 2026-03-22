#include "global.h"

#include "portable/firered_portable_rom_egg_moves_table.h"

#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

#include "data/pokemon/egg_moves.h"

const u16 *FireredEggMovesTable(void)
{
    return gEggMoves;
}

u32 FireredEggMovesTableWordCount(void)
{
    return (u32)NELEMS(gEggMoves);
}

void firered_portable_rom_egg_moves_table_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/moves.h"
#include "constants/species.h"
#include "engine_internal.h"

#define FIRERED_ROM_EGG_MOVES_PACK_MAGIC 0x45474745u /* 'EGGE' LE */
#define FIRERED_ROM_EGG_MOVES_PACK_VERSION 1u
#define EGG_MOVES_SPECIES_OFFSET 20000
#define EGG_MOVES_TERMINATOR 0xFFFFu
#define FIRERED_EGG_MOVES_MAX_U16_WORDS 65536u

static const FireredRomU32TableProfileRow s_egg_moves_profile_rows[] = {
    { 0u, "__firered_builtin_egg_moves_profile_none__", 0u },
};

extern const u16 gEggMoves[];

static u16 *s_host_words;
static u32 s_host_word_count;
static u8 s_egg_moves_rom_active;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void free_host(void)
{
    free(s_host_words);
    s_host_words = NULL;
    s_host_word_count = 0u;
    s_egg_moves_rom_active = 0;
}

static u32 compiled_egg_moves_word_count(void)
{
    u32 i;

    for (i = 0u; i < FIRERED_EGG_MOVES_MAX_U16_WORDS; i++)
    {
        if (gEggMoves[i] == (u16)EGG_MOVES_TERMINATOR)
            return i + 1u;
    }
    return 1u;
}

static bool8 validate_egg_moves_u16_stream(const u16 *words, u32 n)
{
    u32 i = 0u;
    u32 lo = (u32)EGG_MOVES_SPECIES_OFFSET + 1u;
    u32 hi_excl = (u32)EGG_MOVES_SPECIES_OFFSET + (u32)NUM_SPECIES;

    if (n == 0u)
        return FALSE;

    while (i < n)
    {
        u32 w = words[i];

        if (w == EGG_MOVES_TERMINATOR)
            return (bool8)(i == n - 1u);
        if (!(lo <= w && w < hi_excl))
            return FALSE;
        i++;
        while (i < n)
        {
            w = words[i];
            if (w == EGG_MOVES_TERMINATOR)
                return (bool8)(i == n - 1u);
            if (lo <= w && w < hi_excl)
                break;
            if (w >= (u32)MOVES_COUNT)
                return FALSE;
            i++;
        }
    }
    return FALSE;
}

static bool8 load_from_rom(const u8 *rom, size_t rom_size, size_t pack_off)
{
    u32 magic;
    u32 version;
    u32 byte_len;
    u32 n_u16;
    u32 i;
    u16 *dst;

    if (pack_off > rom_size || pack_off + 12u > rom_size)
        return FALSE;

    magic = read_le_u32(rom + pack_off);
    version = read_le_u32(rom + pack_off + 4u);
    byte_len = read_le_u32(rom + pack_off + 8u);

    if (magic != FIRERED_ROM_EGG_MOVES_PACK_MAGIC || version != FIRERED_ROM_EGG_MOVES_PACK_VERSION)
        return FALSE;
    if (byte_len == 0u || (byte_len % 2u) != 0u)
        return FALSE;
    if (byte_len > FIRERED_EGG_MOVES_MAX_U16_WORDS * 2u)
        return FALSE;
    if (pack_off + 12u > rom_size - (size_t)byte_len)
        return FALSE;

    n_u16 = byte_len / 2u;
    dst = (u16 *)malloc((size_t)byte_len);
    if (dst == NULL)
        return FALSE;

    for (i = 0u; i < n_u16; i++)
    {
        const u8 *p = rom + pack_off + 12u + (size_t)i * 2u;

        dst[i] = (u16)(p[0] | (p[1] << 8));
    }

    if (!validate_egg_moves_u16_stream(dst, n_u16))
    {
        free(dst);
        return FALSE;
    }

    free_host();
    s_host_words = dst;
    s_host_word_count = n_u16;
    s_egg_moves_rom_active = 1;
    return TRUE;
}

void firered_portable_rom_egg_moves_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;

    free_host();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_EGG_MOVES_PACK_OFF",
            s_egg_moves_profile_rows, NELEMS(s_egg_moves_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    (void)load_from_rom(rom, rom_size, pack_off);
}

const u16 *FireredEggMovesTable(void)
{
    if (s_egg_moves_rom_active && s_host_words != NULL)
        return s_host_words;
    return gEggMoves;
}

u32 FireredEggMovesTableWordCount(void)
{
    if (s_egg_moves_rom_active && s_host_words != NULL)
        return s_host_word_count;
    return compiled_egg_moves_word_count();
}

#endif /* PORTABLE */
