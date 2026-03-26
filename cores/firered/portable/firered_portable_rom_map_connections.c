#include "global.h"

#include "map_connections_access.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_map_connections_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/global.h"
#include "constants/map_groups.h"
#include "engine_internal.h"

#define FIRERED_ROM_MAP_CONNECTIONS_MAPNUM_MAX 256u
#define FIRERED_ROM_MAP_CONNECTIONS_MAX 8u
#define FIRERED_ROM_MAP_CONNECTIONS_ROW_BYTES (4u + FIRERED_ROM_MAP_CONNECTIONS_MAX * 12u)

_Static_assert(sizeof(struct MapConnection) == 12u, "MapConnection wire stride must match ROM pack (12 bytes)");

static const FireredRomU32TableProfileRow s_map_connections_profile_rows[] = {
    { 0u, "__firered_builtin_map_connections_profile_none__", 0u },
};

extern const struct MapHeader *const *gMapGroups[];
extern const u8 gMapGroupSizes[];

static const u8 *s_pack_base;
static u8 s_map_connections_rom_active;

static struct MapConnection s_rom_conn_list[FIRERED_ROM_MAP_CONNECTIONS_MAX];
static struct MapConnections s_rom_map_connections;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void clear_active(void)
{
    s_pack_base = NULL;
    s_map_connections_rom_active = 0;
}

static size_t pack_byte_size(void)
{
    return (size_t)MAP_GROUPS_COUNT * (size_t)FIRERED_ROM_MAP_CONNECTIONS_MAPNUM_MAX * (size_t)FIRERED_ROM_MAP_CONNECTIONS_ROW_BYTES;
}

static const u8 *row_ptr(u16 mapGroup, u16 mapNum)
{
    size_t idx;

    if (!s_map_connections_rom_active || s_pack_base == NULL)
        return NULL;
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= FIRERED_ROM_MAP_CONNECTIONS_MAPNUM_MAX)
        return NULL;
    idx = (size_t)mapGroup * (size_t)FIRERED_ROM_MAP_CONNECTIONS_MAPNUM_MAX + (size_t)mapNum;
    return s_pack_base + idx * (size_t)FIRERED_ROM_MAP_CONNECTIONS_ROW_BYTES;
}

static bool8 row_all_zero(const u8 *row)
{
    static const u8 z[FIRERED_ROM_MAP_CONNECTIONS_ROW_BYTES];

    return (bool8)(memcmp(row, z, sizeof(z)) == 0);
}

static bool8 connection_slot_valid(const u8 *slot, bool8 active_slot)
{
    struct MapConnection tmp;

    memcpy(&tmp, slot, sizeof(tmp));
    if (!active_slot)
        return (bool8)(memcmp(slot, "\0\0\0\0\0\0\0\0\0\0\0\0", 12) == 0);

    if (tmp.direction < CONNECTION_SOUTH || tmp.direction > CONNECTION_EMERGE)
        return FALSE;
    if (tmp.mapGroup >= MAP_GROUPS_COUNT)
        return FALSE;
    if (tmp.mapNum >= gMapGroupSizes[tmp.mapGroup])
        return FALSE;
    if (gMapGroups[tmp.mapGroup][tmp.mapNum] == NULL)
        return FALSE;
    return TRUE;
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

        for (n = 0; n < (u16)FIRERED_ROM_MAP_CONNECTIONS_MAPNUM_MAX; n++)
        {
            const u8 *row = rom + pack_off
                + ((size_t)g * (size_t)FIRERED_ROM_MAP_CONNECTIONS_MAPNUM_MAX + (size_t)n) * (size_t)FIRERED_ROM_MAP_CONNECTIONS_ROW_BYTES;
            u32 head;
            u32 count;
            u32 i;

            if (n >= maxn)
            {
                if (!row_all_zero(row))
                    return FALSE;
                continue;
            }

            if (gMapGroups[g][n] == NULL)
                return FALSE;

            head = read_le_u32(row);
            count = head & 0xFFu;
            if ((head & ~0xFFu) != 0u)
                return FALSE;
            if (count > FIRERED_ROM_MAP_CONNECTIONS_MAX)
                return FALSE;

            for (i = 0u; i < FIRERED_ROM_MAP_CONNECTIONS_MAX; i++)
            {
                const u8 *slot = row + 4u + (size_t)i * 12u;

                if (!connection_slot_valid(slot, (bool8)(i < count)))
                    return FALSE;
            }
        }
    }

    return TRUE;
}

void firered_portable_rom_map_connections_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;

    clear_active();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_CONNECTIONS_PACK_OFF",
            s_map_connections_profile_rows, NELEMS(s_map_connections_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (!validate_pack(rom_size, rom, pack_off))
        return;

    s_pack_base = rom + pack_off;
    s_map_connections_rom_active = 1;
}

void firered_portable_rom_map_connections_merge(struct MapHeader *dst, u16 mapGroup, u16 mapNum)
{
    const u8 *row;
    u32 head;
    u32 count;
    u32 i;

    if (dst == NULL || !s_map_connections_rom_active)
        return;
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= FIRERED_ROM_MAP_CONNECTIONS_MAPNUM_MAX)
        return;
    if (mapNum >= gMapGroupSizes[mapGroup])
        return;

    row = row_ptr(mapGroup, mapNum);
    if (row == NULL)
        return;

    head = read_le_u32(row);
    count = head & 0xFFu;
    if ((head & ~0xFFu) != 0u || count > FIRERED_ROM_MAP_CONNECTIONS_MAX)
        return;

    for (i = 0u; i < count; i++)
        memcpy(&s_rom_conn_list[i], row + 4u + (size_t)i * 12u, sizeof(struct MapConnection));

    if (count == 0u)
    {
        dst->connections = NULL;
        return;
    }

    s_rom_map_connections.count = (s32)count;
    s_rom_map_connections.connections = s_rom_conn_list;
    dst->connections = &s_rom_map_connections;
}

#endif /* PORTABLE */
