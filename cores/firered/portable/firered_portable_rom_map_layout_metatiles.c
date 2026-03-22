#include "global.h"

#include "map_layout_metatiles_access.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_map_layout_metatiles_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

#define FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS 512u

typedef struct MapLayoutMetatileDirEntryWire {
    u32 border_off;
    u32 map_off;
    u32 border_words;
    u32 map_words;
} MapLayoutMetatileDirEntryWire;

typedef struct LayoutPtrSlot {
    uintptr_t ptr;
    unsigned idx;
} LayoutPtrSlot;

static const FireredRomU32TableProfileRow s_map_layout_metatiles_profile_rows[] = {
    { 0u, "__firered_builtin_map_layout_metatiles_profile_none__", 0u },
};

extern const struct MapLayout *gMapLayouts[];
extern const u32 gMapLayoutsSlotCount;

static const u16 *s_border_ptr[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static const u16 *s_map_ptr[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static LayoutPtrSlot s_layout_ptr_slots[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static unsigned s_layout_ptr_slot_count;
static u8 s_map_layout_metatiles_rom_active;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static int cmp_layout_ptr_slot(const void *a, const void *b)
{
    const LayoutPtrSlot *sa = (const LayoutPtrSlot *)a;
    const LayoutPtrSlot *sb = (const LayoutPtrSlot *)b;

    if (sa->ptr < sb->ptr)
        return -1;
    if (sa->ptr > sb->ptr)
        return 1;
    return 0;
}

static int cmp_layout_key_to_slot(const void *key_ptr, const void *elem_ptr)
{
    uintptr_t key = *(const uintptr_t *)key_ptr;
    const LayoutPtrSlot *slot = (const LayoutPtrSlot *)elem_ptr;

    if (key < slot->ptr)
        return -1;
    if (key > slot->ptr)
        return 1;
    return 0;
}

static void clear_active_state(void)
{
    memset(s_border_ptr, 0, sizeof(s_border_ptr));
    memset(s_map_ptr, 0, sizeof(s_map_ptr));
    s_layout_ptr_slot_count = 0u;
    s_map_layout_metatiles_rom_active = 0;
}

static int layout_index_lookup(const struct MapLayout *layout)
{
    uintptr_t key;
    const LayoutPtrSlot *found;

    if (layout == NULL || s_layout_ptr_slot_count == 0u)
        return -1;

    key = (uintptr_t)layout;
    found = bsearch(&key, s_layout_ptr_slots, s_layout_ptr_slot_count, sizeof(LayoutPtrSlot), cmp_layout_key_to_slot);
    if (found == NULL)
        return -1;
    return (int)found->idx;
}

void firered_portable_rom_map_layout_metatiles_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t dir_off;
    u32 slot_count;
    size_t dir_bytes;
    size_t i;

    clear_active_state();

    slot_count = gMapLayoutsSlotCount;
    if (slot_count == 0u || slot_count > FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS)
        return;

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF",
            s_map_layout_metatiles_profile_rows, NELEMS(s_map_layout_metatiles_profile_rows), &dir_off))
        return;

    dir_bytes = (size_t)slot_count * sizeof(MapLayoutMetatileDirEntryWire);

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (dir_off > rom_size || dir_bytes > rom_size - dir_off)
        return;

    for (i = 0u; i < (size_t)slot_count; i++)
    {
        const u8 *row = rom + dir_off + i * sizeof(MapLayoutMetatileDirEntryWire);
        u32 bo = read_le_u32(row + 0u);
        u32 mo = read_le_u32(row + 4u);
        u32 bw = read_le_u32(row + 8u);
        u32 mw = read_le_u32(row + 12u);
        const struct MapLayout *ml = gMapLayouts[i];

        if (ml == NULL)
        {
            if (bo != 0u || mo != 0u || bw != 0u || mw != 0u)
                goto fail;
            continue;
        }

        {
            u32 expect_bw;
            u32 expect_mw;
            size_t border_bytes;
            size_t map_bytes;

            if (ml->borderWidth == 0 || ml->borderHeight == 0 || ml->width <= 0 || ml->height <= 0)
                goto fail;

            expect_bw = (u32)ml->borderWidth * (u32)ml->borderHeight;
            expect_mw = (u32)ml->width * (u32)ml->height;

            if (bw != expect_bw || mw != expect_mw)
                goto fail;

            border_bytes = (size_t)bw * 2u;
            map_bytes = (size_t)mw * 2u;

            if ((bo | mo) & 1u)
                goto fail;
            if (bo > rom_size || border_bytes > rom_size - bo)
                goto fail;
            if (mo > rom_size || map_bytes > rom_size - mo)
                goto fail;

            s_border_ptr[i] = (const u16 *)(const void *)(rom + bo);
            s_map_ptr[i] = (const u16 *)(const void *)(rom + mo);
        }
    }

    s_layout_ptr_slot_count = 0u;
    for (i = 0u; i < (size_t)slot_count; i++)
    {
        if (gMapLayouts[i] != NULL)
        {
            if (s_layout_ptr_slot_count >= FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS)
                goto fail;
            s_layout_ptr_slots[s_layout_ptr_slot_count].ptr = (uintptr_t)gMapLayouts[i];
            s_layout_ptr_slots[s_layout_ptr_slot_count].idx = (unsigned)i;
            s_layout_ptr_slot_count++;
        }
    }

    if (s_layout_ptr_slot_count > 1u)
        qsort(s_layout_ptr_slots, s_layout_ptr_slot_count, sizeof(LayoutPtrSlot), cmp_layout_ptr_slot);

    s_map_layout_metatiles_rom_active = 1;
    return;

fail:
    clear_active_state();
}

const u16 *FireredMapLayoutMetatileMapPtr(const struct MapLayout *layout)
{
    int idx;

    if (layout == NULL)
        return NULL;
    if (!s_map_layout_metatiles_rom_active)
        return layout->map;

    idx = layout_index_lookup(layout);
    if (idx < 0 || (unsigned)idx >= FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS)
        return layout->map;
    if (s_map_ptr[idx] == NULL)
        return layout->map;
    return s_map_ptr[idx];
}

const u16 *FireredMapLayoutMetatileBorderPtr(const struct MapLayout *layout)
{
    int idx;

    if (layout == NULL)
        return NULL;
    if (!s_map_layout_metatiles_rom_active)
        return layout->border;

    idx = layout_index_lookup(layout);
    if (idx < 0 || (unsigned)idx >= FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS)
        return layout->border;
    if (s_border_ptr[idx] == NULL)
        return layout->border;
    return s_border_ptr[idx];
}

#endif /* PORTABLE */
