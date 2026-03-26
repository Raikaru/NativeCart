/* Host ROM pack buffers: engine_backend_init runs this before AgbMain InitHeap. */
#define FIRERED_HOST_LIBC_MALLOC 1

#include "global.h"

#include "portable/firered_portable_rom_facility_class_lookups.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_facility_class_lookups_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/trainers.h"
#include "engine_internal.h"

#define FIRERED_FCLC_PACK_MAGIC 0x46434C43u /* 'FCLC' LE */
#define FIRERED_FCLC_PACK_VERSION 1u
#define FIRERED_FCLC_HEADER_BYTES 16u

#define FIRERED_TRAINER_PIC_INDEX_COUNT ((size_t)TRAINER_PIC_PAINTER + 1u)
#define FIRERED_TRAINER_CLASS_INDEX_COUNT ((size_t)TRAINER_CLASS_PAINTER + 1u)

const u8 *gFacilityClassToPicIndexActive;
const u8 *gFacilityClassToTrainerClassActive;

static const FireredRomU32TableProfileRow s_facility_class_lookups_profile_rows[] = {
    { 0u, "__firered_builtin_facility_class_lookups_profile_none__", 0u },
};

static u8 *s_pool_pic;
static u8 *s_pool_trainer;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void facility_class_lookups_sync_active_ptrs(void)
{
    gFacilityClassToPicIndexActive = s_pool_pic;
    gFacilityClassToTrainerClassActive = s_pool_trainer;
}

static int facility_class_lookups_rows_ok(const u8 *pic, const u8 *tc, u32 n)
{
    u32 i;

    for (i = 0; i < n; i++)
    {
        if ((size_t)pic[i] >= FIRERED_TRAINER_PIC_INDEX_COUNT)
            return 0;
        if ((size_t)tc[i] >= FIRERED_TRAINER_CLASS_INDEX_COUNT)
            return 0;
    }
    return 1;
}

void firered_portable_rom_facility_class_lookups_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;
    u32 magic;
    u32 version;
    u32 entry_count;
    u32 reserved;
    size_t need;
    size_t body;
    u8 *np = NULL;
    u8 *nt = NULL;

    free(s_pool_pic);
    free(s_pool_trainer);
    s_pool_pic = NULL;
    s_pool_trainer = NULL;
    facility_class_lookups_sync_active_ptrs();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_FACILITY_CLASS_LOOKUPS_PACK_OFF",
            s_facility_class_lookups_profile_rows, NELEMS(s_facility_class_lookups_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (pack_off > rom_size - FIRERED_FCLC_HEADER_BYTES)
        return;

    magic = read_le_u32(rom + pack_off);
    version = read_le_u32(rom + pack_off + 4u);
    entry_count = read_le_u32(rom + pack_off + 8u);
    reserved = read_le_u32(rom + pack_off + 12u);

    if (magic != FIRERED_FCLC_PACK_MAGIC || version != FIRERED_FCLC_PACK_VERSION)
        return;
    if (reserved != 0u)
        return;
    if (entry_count != (u32)NUM_FACILITY_CLASSES)
        return;

    body = (size_t)entry_count * 2u;
    need = FIRERED_FCLC_HEADER_BYTES + body;
    if (pack_off > rom_size - need)
        return;

    np = (u8 *)malloc((size_t)entry_count);
    nt = (u8 *)malloc((size_t)entry_count);
    if (np == NULL || nt == NULL)
    {
        free(np);
        free(nt);
        return;
    }

    memcpy(np, rom + pack_off + FIRERED_FCLC_HEADER_BYTES, (size_t)entry_count);
    memcpy(nt, rom + pack_off + FIRERED_FCLC_HEADER_BYTES + (size_t)entry_count, (size_t)entry_count);

    if (!facility_class_lookups_rows_ok(np, nt, entry_count))
    {
        free(np);
        free(nt);
        return;
    }

    s_pool_pic = np;
    s_pool_trainer = nt;
    facility_class_lookups_sync_active_ptrs();
}

#endif /* PORTABLE */
