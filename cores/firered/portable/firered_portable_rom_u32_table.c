#include "global.h"

#include "portable/firered_portable_rom_u32_table.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef PORTABLE

bool8 firered_portable_rom_u32_table_profile_lookup(const FireredRomU32TableProfileRow *rows, size_t row_count,
    size_t *out_off)
{
    (void)rows;
    (void)row_count;
    (void)out_off;
    return FALSE;
}

bool8 firered_portable_rom_u32_table_resolve_base_off(const char *env_var_name,
    const FireredRomU32TableProfileRow *profile_rows, size_t profile_row_count, size_t *out_off)
{
    (void)env_var_name;
    (void)profile_rows;
    (void)profile_row_count;
    (void)out_off;
    return FALSE;
}

bool8 firered_portable_rom_u32_table_read_entry(size_t rom_size, const u8 *rom, size_t table_off, size_t entry_count,
    size_t entry_index, u32 *out_word)
{
    (void)rom_size;
    (void)rom;
    (void)table_off;
    (void)entry_count;
    (void)entry_index;
    (void)out_word;
    return FALSE;
}

#else

#include "portable/firered_portable_rom_asset_bind.h"
#include "portable/firered_portable_rom_compat.h"

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

bool8 firered_portable_rom_u32_table_profile_lookup(const FireredRomU32TableProfileRow *rows, size_t row_count,
    size_t *out_off)
{
    const FireredRomCompatInfo *c;
    size_t i;

    if (rows == NULL || out_off == NULL || row_count == 0u)
        return FALSE;

    c = firered_portable_rom_compat_get();
    if (c == NULL)
        return FALSE;

    for (i = 0u; i < row_count; i++)
    {
        const FireredRomU32TableProfileRow *r = &rows[i];

        if (r->rom_crc32 == 0u && r->profile_id == NULL)
            continue;

        if (r->rom_crc32 != 0u && r->rom_crc32 != c->rom_crc32)
            continue;

        if (r->profile_id != NULL
            && (c->profile_id == NULL || strcmp(r->profile_id, (const char *)c->profile_id) != 0))
            continue;

        if (r->table_file_off == 0u)
            continue;

        *out_off = r->table_file_off;
        return TRUE;
    }

    return FALSE;
}

bool8 firered_portable_rom_u32_table_resolve_base_off(const char *env_var_name,
    const FireredRomU32TableProfileRow *profile_rows, size_t profile_row_count, size_t *out_off)
{
    size_t off = 0;

    if (out_off == NULL)
        return FALSE;

    if (env_var_name != NULL && firered_portable_rom_asset_parse_hex_env(env_var_name, &off))
    {
        *out_off = off;
        return TRUE;
    }

    if (profile_rows != NULL && profile_row_count > 0u)
        return firered_portable_rom_u32_table_profile_lookup(profile_rows, profile_row_count, out_off);

    return FALSE;
}

bool8 firered_portable_rom_u32_table_read_entry(size_t rom_size, const u8 *rom, size_t table_off, size_t entry_count,
    size_t entry_index, u32 *out_word)
{
    size_t need;

    if (out_word == NULL || rom == NULL)
        return FALSE;

    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES)
        return FALSE;

    if (entry_count == 0u || entry_index >= entry_count)
        return FALSE;

    if (entry_count > (size_t)(SIZE_MAX / 4u))
        return FALSE;

    need = 4u * entry_count;

    if (table_off > rom_size || table_off + need > rom_size)
        return FALSE;

    *out_word = read_le_u32(rom + table_off + 4u * entry_index);
    return TRUE;
}

#endif /* PORTABLE */
