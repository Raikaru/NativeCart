/* Host ROM pack buffers: engine_backend_init runs this before AgbMain InitHeap. */
#define FIRERED_HOST_LIBC_MALLOC 1

#include "global.h"

#include "portable/firered_portable_rom_mon_pic_layout.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_mon_pic_layout_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/species.h"
#include "data.h"
#include "engine_internal.h"

#define FIRERED_MPIC_PACK_MAGIC 0x4349504Du /* 'MPIC' LE */
#define FIRERED_MPIC_PACK_VERSION 1u
#define FIRERED_MPIC_HEADER_BYTES 24u

_Static_assert(sizeof(struct MonCoords) == 2u, "MonCoords ROM wire is 2 bytes");

const struct MonCoords *gMonFrontPicCoordsActive;
const struct MonCoords *gMonBackPicCoordsActive;
const u8 *gEnemyMonElevationActive;

static const FireredRomU32TableProfileRow s_mon_pic_layout_profile_rows[] = {
    { 0u, "__firered_builtin_mon_pic_layout_profile_none__", 0u },
};

static struct MonCoords *s_pool_front;
static struct MonCoords *s_pool_back;
static u8 *s_pool_elev;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void mon_pic_layout_sync_active_ptrs(void)
{
    gMonFrontPicCoordsActive = s_pool_front;
    gMonBackPicCoordsActive = s_pool_back;
    gEnemyMonElevationActive = s_pool_elev;
}

static int mon_coords_row_ok(const struct MonCoords *c)
{
    u8 w = (u8)((c->size >> 4) & 0xFu);
    u8 h = (u8)(c->size & 0xFu);

    if (w == 0u || h == 0u)
        return 0;
    if (w > 15u || h > 15u)
        return 0;
    return 1;
}

static int mon_pic_layout_pools_ok(const struct MonCoords *front, const struct MonCoords *back, u32 coord_n)
{
    u32 i;

    for (i = 0; i < coord_n; i++)
    {
        if (!mon_coords_row_ok(&front[i]))
            return 0;
        if (!mon_coords_row_ok(&back[i]))
            return 0;
    }
    return 1;
}

void firered_portable_rom_mon_pic_layout_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;
    u32 magic;
    u32 version;
    u32 coord_entry_count;
    u32 elev_entry_count;
    u32 coord_row_bytes;
    u32 reserved0;
    size_t need;
    size_t body;
    struct MonCoords *nf = NULL;
    struct MonCoords *nb = NULL;
    u8 *ne = NULL;

    free(s_pool_front);
    free(s_pool_back);
    free(s_pool_elev);
    s_pool_front = NULL;
    s_pool_back = NULL;
    s_pool_elev = NULL;
    mon_pic_layout_sync_active_ptrs();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MON_PIC_LAYOUT_PACK_OFF",
            s_mon_pic_layout_profile_rows, NELEMS(s_mon_pic_layout_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (pack_off > rom_size - FIRERED_MPIC_HEADER_BYTES)
        return;

    magic = read_le_u32(rom + pack_off);
    version = read_le_u32(rom + pack_off + 4u);
    coord_entry_count = read_le_u32(rom + pack_off + 8u);
    elev_entry_count = read_le_u32(rom + pack_off + 12u);
    coord_row_bytes = read_le_u32(rom + pack_off + 16u);
    reserved0 = read_le_u32(rom + pack_off + 20u);

    if (magic != FIRERED_MPIC_PACK_MAGIC || version != FIRERED_MPIC_PACK_VERSION)
        return;
    if (reserved0 != 0u)
        return;
    /*
     * Front/back tables are sized through SPECIES_UNOWN_QMARK; elevation matches NUM_SPECIES only
     * (see static asserts in data.c).
     */
    if (coord_entry_count != (u32)(SPECIES_UNOWN_QMARK + 1))
        return;
    if (elev_entry_count != (u32)NUM_SPECIES)
        return;
    if (coord_row_bytes != (u32)sizeof(struct MonCoords))
        return;

    body = (size_t)coord_entry_count * (size_t)coord_row_bytes * 2u + (size_t)elev_entry_count;
    need = FIRERED_MPIC_HEADER_BYTES + body;
    if (pack_off > rom_size - need)
        return;

    nf = (struct MonCoords *)malloc((size_t)coord_entry_count * sizeof(struct MonCoords));
    nb = (struct MonCoords *)malloc((size_t)coord_entry_count * sizeof(struct MonCoords));
    ne = (u8 *)malloc((size_t)elev_entry_count);
    if (nf == NULL || nb == NULL || ne == NULL)
    {
        free(nf);
        free(nb);
        free(ne);
        return;
    }

    memcpy(nf, rom + pack_off + FIRERED_MPIC_HEADER_BYTES, (size_t)coord_entry_count * (size_t)coord_row_bytes);
    memcpy(nb, rom + pack_off + FIRERED_MPIC_HEADER_BYTES + (size_t)coord_entry_count * (size_t)coord_row_bytes,
        (size_t)coord_entry_count * (size_t)coord_row_bytes);
    memcpy(ne,
        rom + pack_off + FIRERED_MPIC_HEADER_BYTES + (size_t)coord_entry_count * (size_t)coord_row_bytes * 2u,
        (size_t)elev_entry_count);

    if (!mon_pic_layout_pools_ok(nf, nb, coord_entry_count))
    {
        free(nf);
        free(nb);
        free(ne);
        return;
    }

    s_pool_front = nf;
    s_pool_back = nb;
    s_pool_elev = ne;
    mon_pic_layout_sync_active_ptrs();
}

#endif /* PORTABLE */
