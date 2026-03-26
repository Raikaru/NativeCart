/* Host ROM pack buffers: engine_backend_init runs this before AgbMain InitHeap. */
#define FIRERED_HOST_LIBC_MALLOC 1

#include "global.h"

#include "portable/firered_portable_rom_wild_encounter_family.h"

#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_wild_encounter_family_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_u32_table.h"

#include "constants/species.h"
#include "constants/maps.h"
#include "engine_internal.h"
#include "wild_encounter.h"

#define FIRERED_ROM_WILD_PACK_MAGIC 0x464E4957u /* 'WINF' LE */
#define FIRERED_ROM_WILD_PACK_VERSION 1u
#define FIRERED_GBA_HEADER_ROW_BYTES 20u
#define FIRERED_GBA_WILD_INFO_BYTES 8u
#define FIRERED_GBA_WILD_MON_BYTES 4u

#define FIRERED_WILD_MAX_HEADERS 2048u
#define FIRERED_WILD_MAX_INFOS 8192u
#define FIRERED_WILD_MAX_MONS 65536u

static const FireredRomU32TableProfileRow s_wild_encounter_profile_rows[] = {
    { 0u, "__firered_builtin_wild_encounter_family_profile_none__", 0u },
};

extern const struct WildPokemonHeader gWildMonHeaders[];

static struct WildPokemonHeader *s_host_headers;
static struct WildPokemonInfo *s_host_infos;
static struct WildPokemon *s_host_mons;
static u32 s_host_header_count;
static u8 s_wild_rom_family_active;

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void free_built_family(void)
{
    free(s_host_headers);
    free(s_host_infos);
    free(s_host_mons);
    s_host_headers = NULL;
    s_host_infos = NULL;
    s_host_mons = NULL;
    s_host_header_count = 0u;
    s_wild_rom_family_active = 0;
}

static const u8 *gba_ptr_to_rom(const u8 *rom, size_t rom_size, u32 gba)
{
    size_t off;

    if (gba == 0u)
        return NULL;
    if (gba < 0x08000000u || gba >= 0x0A000000u)
        return NULL;
    off = (size_t)(gba - 0x08000000u);
    if (off >= rom_size)
        return NULL;
    return rom + off;
}

static bool8 species_sane(u16 sp)
{
    if (sp == SPECIES_NONE)
        return TRUE;
    if (sp > NUM_SPECIES)
        return FALSE;
    return TRUE;
}

static bool8 validate_info_chain(const u8 *rom, size_t rom_size, u32 info_gba, u8 slot_count)
{
    const u8 *ip;
    u32 mon_gba;
    const u8 *mp;
    u8 j;

    ip = (const u8 *)gba_ptr_to_rom(rom, rom_size, info_gba);
    if (ip == NULL)
        return FALSE;
    if ((size_t)(ip - rom) + FIRERED_GBA_WILD_INFO_BYTES > rom_size)
        return FALSE;

    mon_gba = read_le_u32(ip + 4u);
    mp = (const u8 *)gba_ptr_to_rom(rom, rom_size, mon_gba);
    if (mp == NULL)
        return FALSE;
    if ((size_t)(mp - rom) + (size_t)slot_count * FIRERED_GBA_WILD_MON_BYTES > rom_size)
        return FALSE;

    for (j = 0; j < slot_count; j++)
    {
        const u8 *row = mp + (size_t)j * FIRERED_GBA_WILD_MON_BYTES;
        u16 species = (u16)(row[2] | (row[3] << 8));

        if (!species_sane(species))
            return FALSE;
    }
    return TRUE;
}

static bool8 validate_header_row(const u8 *rom, size_t rom_size, const u8 *row)
{
    u32 land, water, rock, fish;

    if ((size_t)(row - rom) + FIRERED_GBA_HEADER_ROW_BYTES > rom_size)
        return FALSE;
    if (row[2] != 0u || row[3] != 0u)
        return FALSE;

    land = read_le_u32(row + 4u);
    water = read_le_u32(row + 8u);
    rock = read_le_u32(row + 12u);
    fish = read_le_u32(row + 16u);

    if (land != 0u && !validate_info_chain(rom, rom_size, land, LAND_WILD_COUNT))
        return FALSE;
    if (water != 0u && !validate_info_chain(rom, rom_size, water, WATER_WILD_COUNT))
        return FALSE;
    if (rock != 0u && !validate_info_chain(rom, rom_size, rock, ROCK_WILD_COUNT))
        return FALSE;
    if (fish != 0u && !validate_info_chain(rom, rom_size, fish, FISH_WILD_COUNT))
        return FALSE;
    return TRUE;
}

static bool8 wire_sentinel(const u8 *row)
{
    return (bool8)(row[0] == (u8)MAP_GROUP(MAP_UNDEFINED) && row[1] == (u8)MAP_NUM(MAP_UNDEFINED));
}

static bool8 count_family(const u8 *rom, size_t rom_size, size_t pack_off, u32 header_bytes,
    u32 *out_headers, u32 *out_infos, u32 *out_mons)
{
    u32 hcount;
    u32 icount;
    u32 mcount;
    size_t pos;

    *out_headers = 0u;
    *out_infos = 0u;
    *out_mons = 0u;

    if (header_bytes == 0u || (header_bytes % FIRERED_GBA_HEADER_ROW_BYTES) != 0u)
        return FALSE;
    if (pack_off > rom_size || header_bytes > rom_size - pack_off)
        return FALSE;

    hcount = header_bytes / FIRERED_GBA_HEADER_ROW_BYTES;
    if (hcount == 0u || hcount > FIRERED_WILD_MAX_HEADERS)
        return FALSE;

    icount = 0u;
    mcount = 0u;

    for (pos = 0u; pos < (size_t)header_bytes; pos += FIRERED_GBA_HEADER_ROW_BYTES)
    {
        const u8 *row = rom + pack_off + pos;
        u32 land, water, rock, fish;

        if (!validate_header_row(rom, rom_size, row))
            return FALSE;

        if (wire_sentinel(row))
        {
            if (pos + FIRERED_GBA_HEADER_ROW_BYTES != (size_t)header_bytes)
                return FALSE;
            *out_headers = hcount;
            *out_infos = icount;
            *out_mons = mcount;
            return TRUE;
        }

        land = read_le_u32(row + 4u);
        water = read_le_u32(row + 8u);
        rock = read_le_u32(row + 12u);
        fish = read_le_u32(row + 16u);

        if (land != 0u)
        {
            icount++;
            mcount += LAND_WILD_COUNT;
        }
        if (water != 0u)
        {
            icount++;
            mcount += WATER_WILD_COUNT;
        }
        if (rock != 0u)
        {
            icount++;
            mcount += ROCK_WILD_COUNT;
        }
        if (fish != 0u)
        {
            icount++;
            mcount += FISH_WILD_COUNT;
        }
    }

    return FALSE;
}

static struct WildPokemon *copy_mon_table(const u8 *rom, size_t rom_size, u32 mon_gba, u8 slot_count,
    struct WildPokemon *dst)
{
    const u8 *mp = (const u8 *)gba_ptr_to_rom(rom, rom_size, mon_gba);
    u8 j;

    if (mp == NULL)
        return NULL;

    for (j = 0; j < slot_count; j++)
    {
        const u8 *row = mp + (size_t)j * FIRERED_GBA_WILD_MON_BYTES;

        dst[j].minLevel = row[0];
        dst[j].maxLevel = row[1];
        dst[j].species = (u16)(row[2] | (row[3] << 8));
    }
    return dst + slot_count;
}

static struct WildPokemonInfo *build_info(const u8 *rom, size_t rom_size, u32 info_gba, u8 slot_count,
    struct WildPokemonInfo *dst, struct WildPokemon **mon_cursor)
{
    const u8 *ip = (const u8 *)gba_ptr_to_rom(rom, rom_size, info_gba);
    u32 mon_gba;

    if (ip == NULL)
        return NULL;

    mon_gba = read_le_u32(ip + 4u);
    dst->encounterRate = ip[0];
    dst->wildPokemon = *mon_cursor;
    *mon_cursor = copy_mon_table(rom, rom_size, mon_gba, slot_count, *mon_cursor);
    if (*mon_cursor == NULL)
        return NULL;
    return dst + 1;
}

static const struct WildPokemonInfo *build_ptr_chain(const u8 *rom, size_t rom_size, u32 gba,
    u8 slot_count, struct WildPokemonInfo **icursor, struct WildPokemon **mcursor)
{
    struct WildPokemonInfo *start;

    if (gba == 0u)
        return NULL;
    start = *icursor;
    *icursor = build_info(rom, rom_size, gba, slot_count, *icursor, mcursor);
    if (*icursor == NULL)
        return NULL;
    return (const struct WildPokemonInfo *)start;
}

static bool8 build_family(const u8 *rom, size_t rom_size, size_t pack_off, u32 header_bytes)
{
    u32 n_h, n_i, n_m;
    struct WildPokemonInfo *ic;
    struct WildPokemon *mc;
    size_t pos;

    if (!count_family(rom, rom_size, pack_off, header_bytes, &n_h, &n_i, &n_m))
        return FALSE;

    free_built_family();

    s_host_headers = (struct WildPokemonHeader *)calloc((size_t)n_h, sizeof(struct WildPokemonHeader));
    s_host_infos = (struct WildPokemonInfo *)calloc((size_t)n_i, sizeof(struct WildPokemonInfo));
    s_host_mons = (struct WildPokemon *)calloc((size_t)n_m, sizeof(struct WildPokemon));

    if ((n_h != 0u && s_host_headers == NULL) || (n_i != 0u && s_host_infos == NULL) || (n_m != 0u && s_host_mons == NULL))
    {
        free_built_family();
        return FALSE;
    }

    ic = s_host_infos;
    mc = s_host_mons;
    s_host_header_count = n_h;

    for (pos = 0u; pos < (size_t)header_bytes; pos += FIRERED_GBA_HEADER_ROW_BYTES)
    {
        const u8 *row = rom + pack_off + pos;
        struct WildPokemonHeader *hh = &s_host_headers[pos / FIRERED_GBA_HEADER_ROW_BYTES];
        u32 land, water, rock, fish;

        hh->mapGroup = row[0];
        hh->mapNum = row[1];

        if (wire_sentinel(row))
        {
            hh->landMonsInfo = NULL;
            hh->waterMonsInfo = NULL;
            hh->rockSmashMonsInfo = NULL;
            hh->fishingMonsInfo = NULL;
            break;
        }

        land = read_le_u32(row + 4u);
        water = read_le_u32(row + 8u);
        rock = read_le_u32(row + 12u);
        fish = read_le_u32(row + 16u);

        hh->landMonsInfo = build_ptr_chain(rom, rom_size, land, LAND_WILD_COUNT, &ic, &mc);
        if (land != 0u && hh->landMonsInfo == NULL)
            goto fail;
        hh->waterMonsInfo = build_ptr_chain(rom, rom_size, water, WATER_WILD_COUNT, &ic, &mc);
        if (water != 0u && hh->waterMonsInfo == NULL)
            goto fail;
        hh->rockSmashMonsInfo = build_ptr_chain(rom, rom_size, rock, ROCK_WILD_COUNT, &ic, &mc);
        if (rock != 0u && hh->rockSmashMonsInfo == NULL)
            goto fail;
        hh->fishingMonsInfo = build_ptr_chain(rom, rom_size, fish, FISH_WILD_COUNT, &ic, &mc);
        if (fish != 0u && hh->fishingMonsInfo == NULL)
            goto fail;
    }

    if ((uintptr_t)(ic - s_host_infos) != (uintptr_t)n_i || (uintptr_t)(mc - s_host_mons) != (uintptr_t)n_m)
        goto fail;

    s_wild_rom_family_active = 1;
    return TRUE;

fail:
    free_built_family();
    return FALSE;
}

void firered_portable_rom_wild_encounter_family_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t pack_off;
    u32 magic;
    u32 version;
    u32 header_bytes;

    free_built_family();

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_WILD_ENCOUNTER_FAMILY_PACK_OFF",
            s_wild_encounter_profile_rows, NELEMS(s_wild_encounter_profile_rows), &pack_off))
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    if (pack_off > rom_size || pack_off + 12u > rom_size)
        return;

    magic = read_le_u32(rom + pack_off);
    version = read_le_u32(rom + pack_off + 4u);
    header_bytes = read_le_u32(rom + pack_off + 8u);

    if (magic != FIRERED_ROM_WILD_PACK_MAGIC || version != FIRERED_ROM_WILD_PACK_VERSION)
        return;

    if (!build_family(rom, rom_size, pack_off + 12u, header_bytes))
        return;
}

const struct WildPokemonHeader *FireredWildMonHeadersTable(void)
{
    if (s_wild_rom_family_active && s_host_headers != NULL)
        return s_host_headers;
    return gWildMonHeaders;
}

#endif /* PORTABLE */
