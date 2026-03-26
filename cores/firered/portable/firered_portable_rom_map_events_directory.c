#include "global.h"

#include "map_events_access.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_map_events_directory_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/event_objects.h"
#include "constants/global.h"
#include "constants/map_groups.h"
#include "constants/event_bg.h"
#include "engine_internal.h"

#define FIRERED_ROM_MAP_EVENTS_MAPNUM_MAX 256u
#define FIRERED_ROM_MAP_EVENTS_ROW_BYTES 8u

#define FIRERED_ROM_MAP_EVENTS_PACK_MAGIC 0x5645504Du /* 'MPEV' LE */
#define FIRERED_ROM_MAP_EVENTS_PACK_VERSION 1u

#define FIRERED_ROM_MAP_EVENTS_HEADER_BYTES 16u

#define FIRERED_ROM_MAP_EVENTS_OBJ_MAX OBJECT_EVENT_TEMPLATES_COUNT
#define FIRERED_ROM_MAP_EVENTS_WARP_MAX 64u
#define FIRERED_ROM_MAP_EVENTS_COORD_MAX 64u
#define FIRERED_ROM_MAP_EVENTS_BG_MAX 64u

#define FIRERED_ROM_MAP_EVENTS_OBJ_WIRE_BYTES 24u
#define FIRERED_ROM_MAP_EVENTS_WARP_WIRE_BYTES 8u
#define FIRERED_ROM_MAP_EVENTS_COORD_WIRE_BYTES 16u
#define FIRERED_ROM_MAP_EVENTS_BG_WIRE_BYTES 12u

_Static_assert(FIRERED_ROM_MAP_EVENTS_ROW_BYTES == 8u, "map events dir row");
_Static_assert(sizeof(struct WarpEvent) == FIRERED_ROM_MAP_EVENTS_WARP_WIRE_BYTES, "WarpEvent wire");
_Static_assert(sizeof(struct ObjectEventTemplate) == FIRERED_ROM_MAP_EVENTS_OBJ_WIRE_BYTES, "ObjectEventTemplate wire (PORTABLE)");

static const FireredRomU32TableProfileRow s_map_events_dir_profile_rows[] = {
    { 0u, "__firered_builtin_map_events_directory_profile_none__", 0u },
};

extern const struct MapHeader *const *gMapGroups[];
extern const u8 gMapGroupSizes[];

static const u8 *s_pack_base;
static const u8 *s_mapped_rom;
static u8 s_map_events_dir_rom_active;

static struct ObjectEventTemplate s_rom_objects[FIRERED_ROM_MAP_EVENTS_OBJ_MAX];
static struct WarpEvent s_rom_warps[FIRERED_ROM_MAP_EVENTS_WARP_MAX];
static struct CoordEvent s_rom_coords[FIRERED_ROM_MAP_EVENTS_COORD_MAX];
static struct BgEvent s_rom_bgs[FIRERED_ROM_MAP_EVENTS_BG_MAX];
static struct MapEvents s_rom_map_events;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static u16 read_le_u16(const u8 *p)
{
    return (u16)p[0] | ((u16)p[1] << 8);
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
    s_map_events_dir_rom_active = 0;
}

static size_t pack_byte_size(void)
{
    return (size_t)MAP_GROUPS_COUNT * (size_t)FIRERED_ROM_MAP_EVENTS_MAPNUM_MAX * (size_t)FIRERED_ROM_MAP_EVENTS_ROW_BYTES;
}

static const u8 *row_ptr(u16 mapGroup, u16 mapNum)
{
    size_t idx;

    if (!s_map_events_dir_rom_active || s_pack_base == NULL)
        return NULL;
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= FIRERED_ROM_MAP_EVENTS_MAPNUM_MAX)
        return NULL;
    idx = (size_t)mapGroup * (size_t)FIRERED_ROM_MAP_EVENTS_MAPNUM_MAX + (size_t)mapNum;
    return s_pack_base + idx * (size_t)FIRERED_ROM_MAP_EVENTS_ROW_BYTES;
}

static bool8 row_all_zero(const u8 *row)
{
    static const u8 z[FIRERED_ROM_MAP_EVENTS_ROW_BYTES];

    return (bool8)(memcmp(row, z, sizeof(z)) == 0);
}

static bool8 bg_kind_uses_hidden_item(u8 kind)
{
    if (kind == BG_EVENT_HIDDEN_ITEM)
        return TRUE;
    /* Matches field_control_avatar.c switch cases for underfoot hidden items. */
    if (kind == 5u || kind == 6u)
        return TRUE;
    /* Secret bases store an id in bgUnion.hiddenItem (see map.json / generate_portable_map_data). */
    if (kind == BG_EVENT_SECRET_BASE)
        return TRUE;
    return FALSE;
}

static bool8 validate_warp(const struct WarpEvent *w)
{
    /* MAP_DYNAMIC / MAP_UNDEFINED (constants/maps.h) — no static gMapGroups entry. */
    if (w->mapGroup == 0x7Fu && w->mapNum == 0x7Fu)
        return TRUE;
    if (w->mapGroup == 0xFFu && w->mapNum == 0xFFu)
        return TRUE;
    if (w->mapGroup >= MAP_GROUPS_COUNT)
        return FALSE;
    if (w->mapNum >= gMapGroupSizes[w->mapGroup])
        return FALSE;
    if (gMapGroups[w->mapGroup][w->mapNum] == NULL)
        return FALSE;
    return TRUE;
}

static bool8 validate_object_row(const u8 *wire)
{
    struct ObjectEventTemplate tmp;

    memcpy(&tmp, wire, sizeof(tmp));

    if (tmp.kind == OBJ_KIND_CLONE)
    {
        if (tmp.script != 0u)
            return FALSE;
        if (tmp.objUnion.clone.targetMapGroup >= MAP_GROUPS_COUNT)
            return FALSE;
        if (tmp.objUnion.clone.targetMapNum >= gMapGroupSizes[tmp.objUnion.clone.targetMapGroup])
            return FALSE;
        if (gMapGroups[tmp.objUnion.clone.targetMapGroup][tmp.objUnion.clone.targetMapNum] == NULL)
            return FALSE;
        return TRUE;
    }

    if (tmp.kind != OBJ_KIND_NORMAL)
        return FALSE;

    if (tmp.script != 0u && firered_portable_resolve_script_ptr(tmp.script) == NULL)
        return FALSE;

    return TRUE;
}

static bool8 validate_coord_wire(const u8 *wire)
{
    u32 scriptWord = read_le_u32(wire + 12u);

    if (read_le_u16(wire + 10u) != 0u)
        return FALSE;

    if (scriptWord != 0u && firered_portable_resolve_script_ptr(scriptWord) == NULL)
        return FALSE;
    return TRUE;
}

static bool8 validate_bg_wire(const u8 *wire)
{
    u8 kind = wire[5];
    u32 payload = read_le_u32(wire + 8u);

    if (read_le_u16(wire + 6u) != 0u)
        return FALSE;

    if (bg_kind_uses_hidden_item(kind))
        return TRUE;

    if (payload != 0u && firered_portable_resolve_script_ptr(payload) == NULL)
        return FALSE;
    return TRUE;
}

static void decode_coord_from_wire(const u8 *wire, struct CoordEvent *out)
{
    u32 scriptWord = read_le_u32(wire + 12u);

    out->x = read_le_u16(wire + 0u);
    out->y = read_le_u16(wire + 2u);
    out->elevation = wire[4];
    out->trigger = read_le_u16(wire + 6u);
    out->index = read_le_u16(wire + 8u);
    out->script = (const u8 *)firered_portable_resolve_script_ptr(scriptWord);
}

static void decode_bg_from_wire(const u8 *wire, struct BgEvent *out)
{
    u8 kind = wire[5];
    u32 payload = read_le_u32(wire + 8u);

    out->x = read_le_u16(wire + 0u);
    out->y = read_le_u16(wire + 2u);
    out->elevation = wire[4];
    out->kind = kind;
    if (bg_kind_uses_hidden_item(kind))
        out->bgUnion.hiddenItem = payload;
    else
        out->bgUnion.script = (const u8 *)firered_portable_resolve_script_ptr(payload);
}

static bool8 validate_blob(size_t rom_size, const u8 *rom, u32 blob_off, u32 blob_len)
{
    u32 obj, warp, coord, bg;
    u32 packed;
    u32 expect;
    const u8 *b;
    size_t pos;
    u32 i;

    if (blob_off == 0u && blob_len == 0u)
        return TRUE;
    if (blob_off == 0u || blob_len == 0u)
        return FALSE;
    if ((size_t)blob_off > rom_size || (size_t)blob_len > rom_size - (size_t)blob_off)
        return FALSE;
    if (blob_len < FIRERED_ROM_MAP_EVENTS_HEADER_BYTES)
        return FALSE;

    b = rom + (size_t)blob_off;
    if (read_le_u32(b + 0u) != FIRERED_ROM_MAP_EVENTS_PACK_MAGIC)
        return FALSE;
    if (read_le_u32(b + 4u) != FIRERED_ROM_MAP_EVENTS_PACK_VERSION)
        return FALSE;

    packed = read_le_u32(b + 8u);
    obj = packed & 0xFFu;
    warp = (packed >> 8) & 0xFFu;
    coord = (packed >> 16) & 0xFFu;
    bg = (packed >> 24) & 0xFFu;

    if (read_le_u32(b + 12u) != 0u)
        return FALSE;

    if (obj > FIRERED_ROM_MAP_EVENTS_OBJ_MAX || warp > FIRERED_ROM_MAP_EVENTS_WARP_MAX
        || coord > FIRERED_ROM_MAP_EVENTS_COORD_MAX || bg > FIRERED_ROM_MAP_EVENTS_BG_MAX)
        return FALSE;

    expect = FIRERED_ROM_MAP_EVENTS_HEADER_BYTES
        + obj * FIRERED_ROM_MAP_EVENTS_OBJ_WIRE_BYTES
        + warp * FIRERED_ROM_MAP_EVENTS_WARP_WIRE_BYTES
        + coord * FIRERED_ROM_MAP_EVENTS_COORD_WIRE_BYTES
        + bg * FIRERED_ROM_MAP_EVENTS_BG_WIRE_BYTES;

    if (expect != blob_len)
        return FALSE;

    pos = FIRERED_ROM_MAP_EVENTS_HEADER_BYTES;
    for (i = 0u; i < obj; i++)
    {
        if (!validate_object_row(b + pos))
            return FALSE;
        pos += FIRERED_ROM_MAP_EVENTS_OBJ_WIRE_BYTES;
    }
    for (i = 0u; i < warp; i++)
    {
        struct WarpEvent wtmp;

        memcpy(&wtmp, b + pos, sizeof(wtmp));
        if (!validate_warp(&wtmp))
            return FALSE;
        pos += FIRERED_ROM_MAP_EVENTS_WARP_WIRE_BYTES;
    }
    for (i = 0u; i < coord; i++)
    {
        if (!validate_coord_wire(b + pos))
            return FALSE;
        pos += FIRERED_ROM_MAP_EVENTS_COORD_WIRE_BYTES;
    }
    for (i = 0u; i < bg; i++)
    {
        if (!validate_bg_wire(b + pos))
            return FALSE;
        pos += FIRERED_ROM_MAP_EVENTS_BG_WIRE_BYTES;
    }

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

        for (n = 0; n < (u16)FIRERED_ROM_MAP_EVENTS_MAPNUM_MAX; n++)
        {
            const u8 *row = rom + pack_off
                + ((size_t)g * (size_t)FIRERED_ROM_MAP_EVENTS_MAPNUM_MAX + (size_t)n) * (size_t)FIRERED_ROM_MAP_EVENTS_ROW_BYTES;
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

            if (!validate_blob(rom_size, rom, off, len))
                return FALSE;
        }
    }

    return TRUE;
}

void firered_portable_rom_map_events_directory_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;

    clear_active();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_EVENTS_DIRECTORY_PACK_OFF",
            s_map_events_dir_profile_rows, NELEMS(s_map_events_dir_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (!validate_pack(rom_size, rom, pack_off))
        return;

    s_mapped_rom = rom;
    s_pack_base = rom + pack_off;
    s_map_events_dir_rom_active = 1;
}

void firered_portable_rom_map_events_directory_merge(struct MapHeader *dst, u16 mapGroup, u16 mapNum)
{
    const u8 *row;
    u32 off;
    u32 len;
    const u8 *b;
    u32 packed;
    u32 obj, warp, coord, bg;
    size_t pos;
    u32 i;

    if (dst == NULL || !s_map_events_dir_rom_active)
        return;
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= FIRERED_ROM_MAP_EVENTS_MAPNUM_MAX)
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

    b = s_mapped_rom + (size_t)off;
    packed = read_le_u32(b + 8u);
    obj = packed & 0xFFu;
    warp = (packed >> 8) & 0xFFu;
    coord = (packed >> 16) & 0xFFu;
    bg = (packed >> 24) & 0xFFu;

    pos = FIRERED_ROM_MAP_EVENTS_HEADER_BYTES;
    for (i = 0u; i < obj; i++)
    {
        memcpy(&s_rom_objects[i], b + pos, sizeof(struct ObjectEventTemplate));
        pos += FIRERED_ROM_MAP_EVENTS_OBJ_WIRE_BYTES;
    }
    for (i = 0u; i < warp; i++)
    {
        memcpy(&s_rom_warps[i], b + pos, sizeof(struct WarpEvent));
        pos += FIRERED_ROM_MAP_EVENTS_WARP_WIRE_BYTES;
    }
    for (i = 0u; i < coord; i++)
    {
        decode_coord_from_wire(b + pos, &s_rom_coords[i]);
        pos += FIRERED_ROM_MAP_EVENTS_COORD_WIRE_BYTES;
    }
    for (i = 0u; i < bg; i++)
    {
        decode_bg_from_wire(b + pos, &s_rom_bgs[i]);
        pos += FIRERED_ROM_MAP_EVENTS_BG_WIRE_BYTES;
    }

    s_rom_map_events.objectEventCount = (u8)obj;
    s_rom_map_events.warpCount = (u8)warp;
    s_rom_map_events.coordEventCount = (u8)coord;
    s_rom_map_events.bgEventCount = (u8)bg;
    s_rom_map_events.objectEvents = (obj != 0u) ? s_rom_objects : NULL;
    s_rom_map_events.warps = (warp != 0u) ? s_rom_warps : NULL;
    s_rom_map_events.coordEvents = (coord != 0u) ? s_rom_coords : NULL;
    s_rom_map_events.bgEvents = (bg != 0u) ? s_rom_bgs : NULL;

    dst->events = &s_rom_map_events;
}

#endif /* PORTABLE */
