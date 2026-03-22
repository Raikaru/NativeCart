#include "global.h"

#include "portable/firered_portable_rom_region_map_mapsec_names.h"

#ifndef PORTABLE

void firered_portable_rom_region_map_mapsec_names_refresh_after_rom_load(void)
{
}

#else

#include "portable/firered_portable_rom_compat.h"
#include "portable/firered_portable_rom_text_family.h"
#include "portable/firered_portable_rom_u32_table.h"

#include "constants/region_map_mapsec_names.h"
#include "region_map_mapsec_names_rom.h"

#include "characters.h"
#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern const u8 *const gRegionMapMapsecNames_Compiled[REGION_MAP_MAPSEC_NAME_ENTRY_COUNT];

const u8 *gRegionMapMapsecNamesResolved[REGION_MAP_MAPSEC_NAME_ENTRY_COUNT];

/* Max bytes from string start to EOS when validating ROM-backed labels. */
#define MAPSEC_NAME_MAX_EOS_SEARCH 256u

/*
 * Optional **pointer table** (vanilla layout): `const u8 *const sMapNames[]` in ROM —
 * `REGION_MAP_MAPSEC_NAME_ENTRY_COUNT` little-endian ARM ROM addresses.
 *
 * Env: FIRERED_ROM_REGION_MAP_MAPSEC_NAME_PTR_TABLE_OFF (hex file offset to table base).
 * Profile: rows below (CRC and/or profile_id + table_file_off).
 */
static const FireredRomU32TableProfileRow s_region_map_mapsec_name_ptr_table_profile_rows[] = {
    { 0u, "__firered_builtin_region_map_mapsec_name_ptr_table_profile_none__", 0u },
};

/*
 * Optional **sparse** overrides: direct **file offsets** to individual GBA-encoded strings
 * when the pointer table is unknown but a few strings moved (fork-maintained, keep tiny).
 */
typedef struct FireredRomMapsecNameStringProfileRow {
    uint32_t rom_crc32;
    const char *profile_id;
    unsigned entry_index;
    size_t string_file_off;
} FireredRomMapsecNameStringProfileRow;

static const FireredRomMapsecNameStringProfileRow s_mapsec_name_string_profile_rows[] = {
    { 0u, "__firered_builtin_region_map_mapsec_name_string_profile_none__", (unsigned)-1, 0u },
};

static int mapsec_name_string_terminates(const u8 *rom, size_t rom_size, size_t off, size_t cap)
{
    size_t i;

    if (rom == NULL || off >= rom_size || cap == 0u)
        return 0;
    for (i = off; i < rom_size && (i - off) < cap; i++)
    {
        if (rom[i] == EOS)
            return (i > off) ? 1 : 0;
    }
    return 0;
}

static bool8 try_bind_all_from_pointer_table(size_t table_off, const u8 *rom, size_t rom_size, const u8 **out)
{
    size_t i;
    u32 addr;
    size_t n;

    if (out == NULL || rom == NULL)
        return FALSE;
    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES)
        return FALSE;

    n = (size_t)REGION_MAP_MAPSEC_NAME_ENTRY_COUNT;
    if (n > (size_t)(SIZE_MAX / 4u))
        return FALSE;
    if (table_off > rom_size || (4u * n) > rom_size - table_off)
        return FALSE;

    for (i = 0; i < n; i++)
    {
        size_t soff;

        if (!firered_portable_rom_u32_table_read_entry(rom_size, rom, table_off, n, i, &addr))
            return FALSE;
        if (addr == 0u)
            return FALSE;
        if (addr < 0x08000000u || addr >= 0x0A000000u)
            return FALSE;

        soff = (size_t)(addr - 0x08000000u);
        if (soff >= rom_size)
            return FALSE;
        if (!mapsec_name_string_terminates(rom, rom_size, soff, MAPSEC_NAME_MAX_EOS_SEARCH))
            return FALSE;

        out[i] = rom + soff;
    }

    return TRUE;
}

static bool8 mapsec_name_string_profile_lookup(unsigned entry_index, size_t *out_off)
{
    const FireredRomCompatInfo *c;
    size_t k;

    if (out_off == NULL)
        return FALSE;

    c = firered_portable_rom_compat_get();
    if (c == NULL)
        return FALSE;

    for (k = 0u; k < NELEMS(s_mapsec_name_string_profile_rows); k++)
    {
        const FireredRomMapsecNameStringProfileRow *r = &s_mapsec_name_string_profile_rows[k];

        if (r->rom_crc32 == 0u && r->profile_id == NULL)
            continue;
        if (r->string_file_off == 0u)
            continue;
        if (r->entry_index != entry_index)
            continue;

        if (r->rom_crc32 != 0u && r->rom_crc32 != c->rom_crc32)
            continue;
        if (r->profile_id != NULL
            && (c->profile_id == NULL || strcmp(r->profile_id, (const char *)c->profile_id) != 0))
            continue;

        *out_off = r->string_file_off;
        return TRUE;
    }

    return FALSE;
}

static bool8 mapsec_name_try_profile_rom_off(unsigned entry_index, size_t *out_off, void *user)
{
    (void)user;
    return mapsec_name_string_profile_lookup(entry_index, out_off);
}

static const u8 *region_map_mapsec_name_get_fallback(unsigned entry_index, void *user)
{
    (void)user;

    if (entry_index >= REGION_MAP_MAPSEC_NAME_ENTRY_COUNT)
        return NULL;
    return gRegionMapMapsecNames_Compiled[entry_index];
}

static const FireredRomTextFamilyParams s_region_map_mapsec_name_params = {
    .trace_env_var = NULL,
    .trace_prefix = NULL,
    .entry_count = REGION_MAP_MAPSEC_NAME_ENTRY_COUNT,
    .env_key_names = NULL,
    .min_rom_size_for_scan = 0x200u,
    .env_offset_max_eos_search = 2048u,
    /* Shortest labels are small (e.g. routes); 6+ bytes incl. EOS reduces bogus scan hits. */
    .scan_min_match_len = 6u,
    .get_fallback = region_map_mapsec_name_get_fallback,
    .try_profile_rom_off = mapsec_name_try_profile_rom_off,
    .try_scan = NULL,
    .user = NULL,
    .trace_warn_multi_scan = FALSE,
};

void firered_portable_rom_region_map_mapsec_names_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t ptr_table_off;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;

    if (firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_REGION_MAP_MAPSEC_NAME_PTR_TABLE_OFF",
            s_region_map_mapsec_name_ptr_table_profile_rows, NELEMS(s_region_map_mapsec_name_ptr_table_profile_rows),
            &ptr_table_off)
        && try_bind_all_from_pointer_table(ptr_table_off, rom, rom_size, gRegionMapMapsecNamesResolved))
        return;

    firered_portable_rom_text_family_bind_all(&s_region_map_mapsec_name_params, gRegionMapMapsecNamesResolved);
}

#endif /* PORTABLE */
