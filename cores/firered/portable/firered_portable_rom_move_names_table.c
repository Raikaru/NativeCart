#include "global.h"

#include "portable/firered_portable_rom_move_names_table.h"

#include <stddef.h>
#include <string.h>

#ifndef PORTABLE

/* TU is PORTABLE-only in the SDL shell source list. */

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "characters.h"
#include "constants/global.h"
#include "constants/moves.h"
#include "data.h"
#include "engine_internal.h"

#define FIRERED_MVN_PACK_MAGIC 0x564D4E4Du /* 'MNMV' LE */
#define FIRERED_MVN_PACK_VERSION 1u
#define FIRERED_MVN_HEADER_BYTES 16u

#define FIRERED_MOVE_NAMES_ROW_BYTES ((size_t)(MOVE_NAME_LENGTH + 1))
#define FIRERED_MOVE_NAMES_BYTE_SIZE ((size_t)MOVES_COUNT * FIRERED_MOVE_NAMES_ROW_BYTES)

static const FireredRomU32TableProfileRow s_move_names_profile_rows[] = {
    { 0u, "__firered_builtin_move_names_profile_none__", 0u },
};

static u8 s_move_names_rom[FIRERED_MOVE_NAMES_BYTE_SIZE];
static u8 s_move_names_rom_active;

extern const u8 gMoveNames_Compiled[][MOVE_NAME_LENGTH + 1];

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static bool8 move_names_rows_ok(const u8 *p, size_t total)
{
    size_t r;
    size_t o;

    if (total != FIRERED_MOVE_NAMES_BYTE_SIZE)
        return FALSE;

    for (r = 0; r < (size_t)MOVES_COUNT; r++)
    {
        bool8 has_eos = FALSE;

        o = r * FIRERED_MOVE_NAMES_ROW_BYTES;
        for (; o < (r + 1u) * FIRERED_MOVE_NAMES_ROW_BYTES; o++)
        {
            if (p[o] == EOS)
            {
                has_eos = TRUE;
                break;
            }
        }
        if (!has_eos)
            return FALSE;
    }
    return TRUE;
}

void firered_portable_rom_move_names_table_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;
    u32 magic;
    u32 version;
    u32 rows;
    u32 row_bytes;
    size_t need;

    s_move_names_rom_active = 0;

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MOVE_NAMES_PACK_OFF",
            s_move_names_profile_rows, NELEMS(s_move_names_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (pack_off > rom_size - FIRERED_MVN_HEADER_BYTES)
        return;

    magic = read_le_u32(rom + pack_off);
    version = read_le_u32(rom + pack_off + 4u);
    rows = read_le_u32(rom + pack_off + 8u);
    row_bytes = read_le_u32(rom + pack_off + 12u);

    if (magic != FIRERED_MVN_PACK_MAGIC || version != FIRERED_MVN_PACK_VERSION)
        return;
    if (rows != (u32)MOVES_COUNT || row_bytes != (u32)FIRERED_MOVE_NAMES_ROW_BYTES)
        return;

    need = FIRERED_MVN_HEADER_BYTES + (size_t)rows * (size_t)row_bytes;
    if (pack_off > rom_size - need)
        return;

    memcpy(s_move_names_rom, rom + pack_off + FIRERED_MVN_HEADER_BYTES, FIRERED_MOVE_NAMES_BYTE_SIZE);

    if (!move_names_rows_ok(s_move_names_rom, FIRERED_MOVE_NAMES_BYTE_SIZE))
        return;

    s_move_names_rom_active = 1;
}

const u8 *FireredMoveNamesBytes(void)
{
    if (s_move_names_rom_active)
        return s_move_names_rom;
    return (const u8 *)&gMoveNames_Compiled[0][0];
}

#endif /* PORTABLE */
