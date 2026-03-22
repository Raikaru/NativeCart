#include "global.h"

#include "portable/firered_portable_rom_asset_profile.h"
#include "portable/firered_portable_rom_compat.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef PORTABLE

bool8 firered_portable_rom_asset_profile_lookup(FireredRomAssetFamily family, FireredRomAssetProfileOffsets *out)
{
    (void)family;
    if (out != NULL)
        memset(out, 0, sizeof(*out));
    return FALSE;
}

#else

typedef struct FireredRomAssetProfileRow {
    uint32_t rom_crc32;
    const char *profile_id;
    FireredRomAssetFamily family;
    uint32_t set_mask;
    size_t pal_off;
    size_t pal2_off;
    size_t tiles_off;
    size_t map_off;
} FireredRomAssetProfileRow;

/*
 * Built-in rows: keep tiny. Prefer profile_id + crc when both are stable.
 *
 * Example (do not ship real offsets without verification):
 *   { 0u, "unbound", FIRERED_ROM_ASSET_FAMILY_MAIN_MENU_PALS,
 *     FIRERED_ROM_ASSET_PROFILE_HAS_PAL | FIRERED_ROM_ASSET_PROFILE_HAS_PAL2,
 *     0x123456u, 0x789ABCu, 0u, 0u },
 *
 * First row is a sentinel so the table is non-empty on all compilers; it never
 * matches a real compat profile_id.
 */
static const FireredRomAssetProfileRow s_rom_asset_profile_rows[] = {
    { 0u, "__firered_builtin_asset_profile_none__", FIRERED_ROM_ASSET_FAMILY_TITLE_LOGO, 0u, 0u, 0u, 0u, 0u },
};

bool8 firered_portable_rom_asset_profile_lookup(FireredRomAssetFamily family, FireredRomAssetProfileOffsets *out)
{
    const FireredRomCompatInfo *c;
    size_t i;

    if (out == NULL)
        return FALSE;

    memset(out, 0, sizeof(*out));
    out->set_mask = 0u;

    if (family >= FIRERED_ROM_ASSET_FAMILY_COUNT)
        return FALSE;

    c = firered_portable_rom_compat_get();
    if (c == NULL)
        return FALSE;

    for (i = 0u; i < NELEMS(s_rom_asset_profile_rows); i++)
    {
        const FireredRomAssetProfileRow *r = &s_rom_asset_profile_rows[i];

        if (r->family != family)
            continue;

        if (r->rom_crc32 == 0u && r->profile_id == NULL)
            continue;

        if (r->rom_crc32 != 0u && r->rom_crc32 != c->rom_crc32)
            continue;

        if (r->profile_id != NULL
            && (c->profile_id == NULL || strcmp(r->profile_id, (const char *)c->profile_id) != 0))
            continue;

        out->set_mask = r->set_mask;
        if ((r->set_mask & FIRERED_ROM_ASSET_PROFILE_HAS_PAL) != 0u)
            out->pal_off = r->pal_off;
        if ((r->set_mask & FIRERED_ROM_ASSET_PROFILE_HAS_PAL2) != 0u)
            out->pal2_off = r->pal2_off;
        if ((r->set_mask & FIRERED_ROM_ASSET_PROFILE_HAS_TILES) != 0u)
            out->tiles_off = r->tiles_off;
        if ((r->set_mask & FIRERED_ROM_ASSET_PROFILE_HAS_MAP) != 0u)
            out->map_off = r->map_off;
        return TRUE;
    }

    return FALSE;
}

#endif /* PORTABLE */
