#include "global.h"

#include "map_scripts_access.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_map_scripts_directory_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/map_groups.h"
#include "engine_internal.h"

#define FIRERED_ROM_MAP_SCRIPTS_MAPNUM_MAX 256u
#define FIRERED_ROM_MAP_SCRIPTS_ROW_BYTES 8u

_Static_assert(FIRERED_ROM_MAP_SCRIPTS_ROW_BYTES == 8u, "map scripts dir row");

static const FireredRomU32TableProfileRow s_map_scripts_dir_profile_rows[] = {
    { 0u, "__firered_builtin_map_scripts_directory_profile_none__", 0u },
};

extern const struct MapHeader *const *gMapGroups[];
extern const u8 gMapGroupSizes[];

static const u8 *s_pack_base;
static const u8 *s_mapped_rom;
static u8 s_map_scripts_dir_rom_active;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void read_row(const u8 *row, u32 *out_off, u32 *out_len)
{
    *out_off = read_le_u32(row);
    *out_len = read_le_u32(row + 4u);
}

static void clear_active(void)
{
    s_pack_base = NULL;
    s_mapped_rom = NULL;
    s_map_scripts_dir_rom_active = 0;
}

static size_t pack_byte_size(void)
{
    return (size_t)MAP_GROUPS_COUNT * (size_t)FIRERED_ROM_MAP_SCRIPTS_MAPNUM_MAX * (size_t)FIRERED_ROM_MAP_SCRIPTS_ROW_BYTES;
}

static const u8 *row_ptr(u16 mapGroup, u16 mapNum)
{
    size_t idx;

    if (!s_map_scripts_dir_rom_active || s_pack_base == NULL)
        return NULL;
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= FIRERED_ROM_MAP_SCRIPTS_MAPNUM_MAX)
        return NULL;
    idx = (size_t)mapGroup * (size_t)FIRERED_ROM_MAP_SCRIPTS_MAPNUM_MAX + (size_t)mapNum;
    return s_pack_base + idx * (size_t)FIRERED_ROM_MAP_SCRIPTS_ROW_BYTES;
}

static bool8 row_all_zero(const u8 *row)
{
    static const u8 z[FIRERED_ROM_MAP_SCRIPTS_ROW_BYTES];

    return (bool8)(memcmp(row, z, sizeof(z)) == 0);
}

/*
 * Map header script table: sequence of 5-byte records (tag + LE u32 pointer word) until tag 0.
 * See MapHeaderGetScriptTable in script.c.
 */
static bool8 map_script_blob_plausible(const u8 *blob, size_t len)
{
    size_t pos = 0;

    if (blob == NULL || len == 0u)
        return FALSE;

    while (pos < len)
    {
        u8 tag = blob[pos];

        if (tag == 0u)
            return TRUE;
        if (pos + 5u > len)
            return FALSE;
        pos += 5u;
    }
    return FALSE;
}

static bool8 validate_pack(size_t rom_size, const u8 *rom, size_t pack_off)
{
    u16 g;
    u16 n;

    if (pack_off > rom_size || pack_byte_size() > rom_size - pack_off)
        return FALSE;

    for (g = 0; g < (u16)MAP_GROUPS_COUNT; g++)
    {
        u8 maxn = gMapGroupSizes[g];

        for (n = 0; n < (u16)FIRERED_ROM_MAP_SCRIPTS_MAPNUM_MAX; n++)
        {
            const u8 *row = rom + pack_off
                + ((size_t)g * (size_t)FIRERED_ROM_MAP_SCRIPTS_MAPNUM_MAX + (size_t)n) * (size_t)FIRERED_ROM_MAP_SCRIPTS_ROW_BYTES;
            u32 off;
            u32 len;

            if (n >= maxn)
            {
                if (!row_all_zero(row))
                    return FALSE;
                continue;
            }

            if (gMapGroups[g][n] == NULL)
                return FALSE;

            read_row(row, &off, &len);

            if (off == 0u && len == 0u)
                continue;

            if (off == 0u || len == 0u)
                return FALSE;
            if ((size_t)off > rom_size || (size_t)len > rom_size - (size_t)off)
                return FALSE;
            if (!map_script_blob_plausible(rom + (size_t)off, (size_t)len))
                return FALSE;
        }
    }

    return TRUE;
}

void firered_portable_rom_map_scripts_directory_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;

    clear_active();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_SCRIPTS_DIRECTORY_PACK_OFF",
            s_map_scripts_dir_profile_rows, NELEMS(s_map_scripts_dir_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (!validate_pack(rom_size, rom, pack_off))
        return;

    s_mapped_rom = rom;
    s_pack_base = rom + pack_off;
    s_map_scripts_dir_rom_active = 1;
}

void firered_portable_rom_map_scripts_directory_merge(struct MapHeader *dst, u16 mapGroup, u16 mapNum)
{
    const u8 *row;
    u32 off;
    u32 len;

    if (dst == NULL || !s_map_scripts_dir_rom_active)
        return;
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= FIRERED_ROM_MAP_SCRIPTS_MAPNUM_MAX)
        return;
    if (mapNum >= gMapGroupSizes[mapGroup])
        return;

    row = row_ptr(mapGroup, mapNum);
    if (row == NULL)
        return;

    read_row(row, &off, &len);
    if (off == 0u && len == 0u)
        return;

    if (s_mapped_rom == NULL)
        return;
    dst->mapScripts = s_mapped_rom + (size_t)off;
}

#endif /* PORTABLE */
