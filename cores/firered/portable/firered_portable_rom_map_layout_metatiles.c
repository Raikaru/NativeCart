#include "global.h"

#include "fieldmap.h"
#include "map_layout_metatiles_access.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef PORTABLE

void firered_portable_rom_map_layout_metatiles_refresh_after_rom_load(void)
{
}

#else
/*
 * ROM metatile directory blobs at `border_off` / `map_off` are **PRET wire V1** `u16` words (same as
 * `map.bin` / `border.bin` on disk). Runtime converts to Phase 2 RAM in `fieldmap.c`.
 */

#include "portable/firered_portable_rom_u32_table.h"

#include "engine_internal.h"

#define FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS 512u
#define FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX 512u

typedef struct MapLayoutMetatileDirEntryWire {
    u32 border_off;
    u32 map_off;
    u32 border_words;
    u32 map_words;
} MapLayoutMetatileDirEntryWire;

typedef struct MapLayoutDimensionsWire {
    u16 width;
    u16 height;
    u16 borderWidth;
    u16 borderHeight;
} MapLayoutDimensionsWire;

/*
 * Offline CI fixtures (``tools/run_offline_build_gates.py``): IEEE CRC32 of the mapped ROM image
 * matches one of these rows → layout companion directory bases auto-bind without env.
 * Goldens: ``tools/build_offline_layout_prose_fixture.py``.
 */
static const FireredRomU32TableProfileRow s_map_layout_metatiles_profile_rows[] = {
    { 0u, "__firered_builtin_map_layout_metatiles_profile_none__", 0u },
    { 0x217A04B1u, NULL, 0u },
    { 0x33B8FD06u, NULL, 0u },
    { 0x8D911746u, NULL, 0x1000000u },
    { 0x3479F247u, NULL, 0x1000000u },
};

static const FireredRomU32TableProfileRow s_map_layout_dimensions_profile_rows[] = {
    { 0u, "__firered_builtin_map_layout_dimensions_profile_none__", 0u },
    { 0x217A04B1u, NULL, 0x788DEu },
    { 0x33B8FD06u, NULL, 0x788DEu },
    { 0x8D911746u, NULL, 0x10788DEu },
    { 0x3479F247u, NULL, 0x10788DEu },
};

static const FireredRomU32TableProfileRow s_map_layout_tileset_indices_profile_rows[] = {
    { 0u, "__firered_builtin_map_layout_tileset_indices_profile_none__", 0u },
    { 0x217A04B1u, NULL, 0x794D6u },
    { 0x33B8FD06u, NULL, 0x794D6u },
    { 0x8D911746u, NULL, 0x10794D6u },
    { 0x3479F247u, NULL, 0x10794D6u },
};

static const FireredRomU32TableProfileRow s_map_layout_metatile_attributes_profile_rows[] = {
    { 0u, "__firered_builtin_map_layout_metatile_attributes_profile_none__", 0u },
};

static const FireredRomU32TableProfileRow s_map_layout_metatile_graphics_profile_rows[] = {
    { 0u, "__firered_builtin_map_layout_metatile_graphics_profile_none__", 0u },
};

static const FireredRomU32TableProfileRow s_compiled_tileset_tiles_profile_rows[] = {
    { 0u, "__firered_builtin_compiled_tileset_tiles_profile_none__", 0u },
};

static const FireredRomU32TableProfileRow s_compiled_tileset_palettes_profile_rows[] = {
    { 0u, "__firered_builtin_compiled_tileset_palettes_profile_none__", 0u },
};

static const FireredRomU32TableProfileRow s_compiled_tileset_asset_profile_profile_rows[] = {
    { 0u, "__firered_builtin_compiled_tileset_asset_profile_none__", 0u },
};

static const FireredRomU32TableProfileRow s_field_map_palette_policy_profile_rows[] = {
    { 0u, "__firered_builtin_field_map_palette_policy_none__", 0u },
};

extern const struct MapLayout *gMapLayouts[];
extern const u32 gMapLayoutsSlotCount;
extern const struct Tileset *const gFireredPortableCompiledTilesetTable[];
extern const u32 gFireredPortableCompiledTilesetTableCount;

#define FIRERED_USE_COMPILED_TILESET_INDEX 0xFFFFu

typedef struct MapLayoutTilesetIndicesWire {
    u16 primary_index;
    u16 secondary_index;
} MapLayoutTilesetIndicesWire;

typedef struct CompiledTilesetAssetProfileWire {
    u32 tile_uncomp_bytes;
    u32 palette_bundle_bytes;
    u32 metatile_gfx_bytes;
    u32 metatile_attr_bytes;
} CompiledTilesetAssetProfileWire;

#define FIRERED_PORTABLE_COMPILED_TILESET_PROFILE_TILE_UNCOMP_MAX (512u * 1024u)
#define FIRERED_PORTABLE_COMPILED_TILESET_PROFILE_PAL_BUNDLE_MAX (256u * (u32)PLTT_SIZE_4BPP)
#define FIRERED_PORTABLE_COMPILED_TILESET_PROFILE_METATILE_GFX_MAX (2048u * (u32)NUM_TILES_PER_METATILE * (u32)sizeof(u16))
#define FIRERED_PORTABLE_COMPILED_TILESET_PROFILE_METATILE_ATTR_MAX (2048u * sizeof(u32))

static const u16 *s_border_ptr[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static const u16 *s_map_ptr[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static u8 s_map_layout_metatiles_rom_active;

static const struct Tileset *s_ts_ov_primary[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static const struct Tileset *s_ts_ov_secondary[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static u8 s_tileset_indices_rom_active;

static const u32 *s_layout_pri_metatile_attr[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static const u32 *s_layout_sec_metatile_attr[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static u8 s_metatile_attributes_rom_active;

static const u16 *s_layout_pri_metatile_gfx[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static const u16 *s_layout_sec_metatile_gfx[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static u8 s_metatile_graphics_rom_active;

static const u32 *s_compiled_ts_tile_data[FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX];
static u8 s_compiled_ts_tiles_rom_active;

static const u16 *s_compiled_ts_pal_data[FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX];
static u8 s_compiled_ts_pals_rom_active;

static u32 s_compiled_ts_prof_tile_uncomp[FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX];
static u32 s_compiled_ts_prof_palette_bundle_bytes[FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX];
static u32 s_compiled_ts_prof_metatile_gfx_bytes[FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX];
static u32 s_compiled_ts_prof_metatile_attr_bytes[FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX];
static u8 s_compiled_ts_asset_profile_active;

/* Effective field BG palette row split (primary rows, then secondary); defaults = vanilla. */
static u8 s_field_map_pal_pri_rows = (u8)NUM_PALS_IN_PRIMARY;
static u8 s_field_map_pal_sec_rows = (u8)(NUM_PALS_TOTAL - NUM_PALS_IN_PRIMARY);
static u8 s_field_map_pal_policy_override_active;

static struct MapLayout s_effective_layout[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static u8 s_effective_valid[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];

/* Per-slot ROM dimension override (only when dimensions directory present + row non-zero). */
static u8 s_dim_rom_active[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static u16 s_dim_rom_w[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static u16 s_dim_rom_h[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static u16 s_dim_rom_bw[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];
static u16 s_dim_rom_bh[FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS];

static u32 read_le_u32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static u16 read_le_u16(const u8 *p)
{
    return (u16)((u16)p[0] | ((u16)p[1] << 8));
}

static void clear_metatile_ptrs(void)
{
    memset(s_border_ptr, 0, sizeof(s_border_ptr));
    memset(s_map_ptr, 0, sizeof(s_map_ptr));
    s_map_layout_metatiles_rom_active = 0;
}

static void clear_tileset_overrides(void)
{
    memset(s_ts_ov_primary, 0, sizeof(s_ts_ov_primary));
    memset(s_ts_ov_secondary, 0, sizeof(s_ts_ov_secondary));
    s_tileset_indices_rom_active = 0;
}

static void clear_metatile_attributes_overlay(void)
{
    memset(s_layout_pri_metatile_attr, 0, sizeof(s_layout_pri_metatile_attr));
    memset(s_layout_sec_metatile_attr, 0, sizeof(s_layout_sec_metatile_attr));
    s_metatile_attributes_rom_active = 0;
}

static void clear_metatile_graphics_overlay(void)
{
    memset(s_layout_pri_metatile_gfx, 0, sizeof(s_layout_pri_metatile_gfx));
    memset(s_layout_sec_metatile_gfx, 0, sizeof(s_layout_sec_metatile_gfx));
    s_metatile_graphics_rom_active = 0;
}

static void clear_compiled_tileset_tiles_overlay(void)
{
    memset(s_compiled_ts_tile_data, 0, sizeof(s_compiled_ts_tile_data));
    s_compiled_ts_tiles_rom_active = 0;
}

static void clear_compiled_tileset_palettes_overlay(void)
{
    memset(s_compiled_ts_pal_data, 0, sizeof(s_compiled_ts_pal_data));
    s_compiled_ts_pals_rom_active = 0;
}

static void clear_compiled_tileset_asset_profile(void)
{
    memset(s_compiled_ts_prof_tile_uncomp, 0, sizeof(s_compiled_ts_prof_tile_uncomp));
    memset(s_compiled_ts_prof_palette_bundle_bytes, 0, sizeof(s_compiled_ts_prof_palette_bundle_bytes));
    memset(s_compiled_ts_prof_metatile_gfx_bytes, 0, sizeof(s_compiled_ts_prof_metatile_gfx_bytes));
    memset(s_compiled_ts_prof_metatile_attr_bytes, 0, sizeof(s_compiled_ts_prof_metatile_attr_bytes));
    s_compiled_ts_asset_profile_active = 0;
}

static void clear_field_map_palette_policy(void)
{
    s_field_map_pal_pri_rows = (u8)NUM_PALS_IN_PRIMARY;
    s_field_map_pal_sec_rows = (u8)(NUM_PALS_TOTAL - NUM_PALS_IN_PRIMARY);
    s_field_map_pal_policy_override_active = 0;
}

static void try_apply_field_map_palette_policy_from_rom(size_t rom_size, const u8 *rom)
{
    size_t off;
    u32 w;
    u32 pri;
    u32 sec;

    if (rom == NULL)
        return;

    if (!firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF",
            s_field_map_palette_policy_profile_rows, NELEMS(s_field_map_palette_policy_profile_rows), &off))
        return;

    if ((off & 3u) != 0u)
    {
        clear_field_map_palette_policy();
        return;
    }
    if (off > rom_size || sizeof(u32) > rom_size - off)
    {
        clear_field_map_palette_policy();
        return;
    }

    w = read_le_u32(rom + off);
    if (w == 0u)
        return;

    pri = w & 0xFFu;
    sec = (w >> 8) & 0xFFu;
    if ((w >> 16) != 0u)
    {
        clear_field_map_palette_policy();
        return;
    }
    if (pri < 1u || sec < 1u)
    {
        clear_field_map_palette_policy();
        return;
    }
    if (pri + sec != (u32)NUM_PALS_TOTAL)
    {
        clear_field_map_palette_policy();
        return;
    }

    s_field_map_pal_pri_rows = (u8)pri;
    s_field_map_pal_sec_rows = (u8)sec;
    s_field_map_pal_policy_override_active = 1;
}

bool8 FireredRomFieldMapPalettePolicyRomActive(void)
{
    return (bool8)(s_field_map_pal_policy_override_active != 0);
}

u8 FireredPortableFieldMapBgPalettePrimaryRows(void)
{
    return s_field_map_pal_pri_rows;
}

u8 FireredPortableFieldMapBgPaletteSecondaryRows(void)
{
    return s_field_map_pal_sec_rows;
}

static void clear_dim_rom_state(void)
{
    memset(s_dim_rom_active, 0, sizeof(s_dim_rom_active));
}

static void rebuild_effective_layouts_compiled_only(void)
{
    u32 i;

    memset(s_effective_valid, 0, sizeof(s_effective_valid));
    for (i = 0u; i < gMapLayoutsSlotCount && i < FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS; i++)
    {
        const struct MapLayout *ml = gMapLayouts[i];

        if (ml == NULL)
            continue;
        memcpy(&s_effective_layout[i], ml, sizeof(struct MapLayout));
        s_effective_valid[i] = 1;
    }
}

/*
 * After metatile pointers (and optional ROM dimensions / tileset indices) are known,
 * refresh per-slot MapLayout overlays.
 */
static void rebuild_effective_layouts_final(void)
{
    u32 i;

    memset(s_effective_valid, 0, sizeof(s_effective_valid));
    for (i = 0u; i < gMapLayoutsSlotCount && i < FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS; i++)
    {
        const struct MapLayout *ml = gMapLayouts[i];

        if (ml == NULL)
            continue;

        memcpy(&s_effective_layout[i], ml, sizeof(struct MapLayout));

        if (s_dim_rom_active[i] != 0)
        {
            s_effective_layout[i].width = (s32)s_dim_rom_w[i];
            s_effective_layout[i].height = (s32)s_dim_rom_h[i];
            s_effective_layout[i].borderWidth = (u8)s_dim_rom_bw[i];
            s_effective_layout[i].borderHeight = (u8)s_dim_rom_bh[i];
        }

        if (s_map_layout_metatiles_rom_active != 0)
        {
            if (s_border_ptr[i] != NULL)
                s_effective_layout[i].border = s_border_ptr[i];
            if (s_map_ptr[i] != NULL)
                s_effective_layout[i].map = s_map_ptr[i];
        }

        if (s_tileset_indices_rom_active != 0)
        {
            if (s_ts_ov_primary[i] != NULL)
                s_effective_layout[i].primaryTileset = s_ts_ov_primary[i];
            if (s_ts_ov_secondary[i] != NULL)
                s_effective_layout[i].secondaryTileset = s_ts_ov_secondary[i];
        }

        s_effective_valid[i] = 1;
    }
}

static int layout_ptr_to_slot(const struct MapLayout *layout)
{
    u32 i;

    if (layout == NULL)
        return -1;
    for (i = 0u; i < gMapLayoutsSlotCount && i < FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS; i++)
    {
        if (gMapLayouts[i] == layout)
            return (int)i;
        if (s_effective_valid[i] != 0 && layout == &s_effective_layout[i])
            return (int)i;
    }
    return -1;
}

static bool8 parse_dimensions_directory(size_t rom_size, const u8 *rom, size_t dim_off, u32 slot_count)
{
    size_t dim_bytes;
    u32 i;

    dim_bytes = (size_t)slot_count * sizeof(MapLayoutDimensionsWire);
    if (dim_off > rom_size || dim_bytes > rom_size - dim_off)
        return FALSE;

    clear_dim_rom_state();

    for (i = 0u; i < slot_count; i++)
    {
        const u8 *row = rom + dim_off + i * sizeof(MapLayoutDimensionsWire);
        u16 w = read_le_u16(row + 0u);
        u16 h = read_le_u16(row + 2u);
        u16 bw = read_le_u16(row + 4u);
        u16 bh = read_le_u16(row + 6u);
        const struct MapLayout *ml = gMapLayouts[i];
        u32 allzero;

        allzero = (u32)(w | h | bw | bh);
        if (ml == NULL)
        {
            if (allzero != 0u)
                return FALSE;
            continue;
        }

        if (allzero == 0u)
        {
            /* Use compiled geometry for this slot. */
            continue;
        }

        /* Map width/height must be positive; borderWidth/borderHeight may be 0 (zero-area border). */
        if (w == 0u || h == 0u)
            return FALSE;
        if (bw > 255u || bh > 255u)
            return FALSE;

        s_dim_rom_active[i] = 1;
        s_dim_rom_w[i] = w;
        s_dim_rom_h[i] = h;
        s_dim_rom_bw[i] = bw;
        s_dim_rom_bh[i] = bh;
    }

    return TRUE;
}

static bool8 try_apply_tileset_indices_from_rom(size_t rom_size, const u8 *rom, size_t ts_off, u32 slot_count)
{
    size_t ts_bytes;
    u32 i;

    if (gFireredPortableCompiledTilesetTableCount == 0u)
        return FALSE;

    ts_bytes = (size_t)slot_count * sizeof(MapLayoutTilesetIndicesWire);
    if (ts_off > rom_size || ts_bytes > rom_size - ts_off)
        return FALSE;

    memset(s_ts_ov_primary, 0, sizeof(s_ts_ov_primary));
    memset(s_ts_ov_secondary, 0, sizeof(s_ts_ov_secondary));

    for (i = 0u; i < slot_count; i++)
    {
        const u8 *row = rom + ts_off + i * sizeof(MapLayoutTilesetIndicesWire);
        u16 pi = read_le_u16(row + 0u);
        u16 si = read_le_u16(row + 2u);
        const struct MapLayout *ml = gMapLayouts[i];

        if (ml == NULL)
        {
            if (pi != FIRERED_USE_COMPILED_TILESET_INDEX || si != FIRERED_USE_COMPILED_TILESET_INDEX)
                return FALSE;
            continue;
        }

        if (pi != FIRERED_USE_COMPILED_TILESET_INDEX)
        {
            if ((u32)pi >= gFireredPortableCompiledTilesetTableCount)
                return FALSE;
            s_ts_ov_primary[i] = gFireredPortableCompiledTilesetTable[pi];
        }

        if (si != FIRERED_USE_COMPILED_TILESET_INDEX)
        {
            if ((u32)si >= gFireredPortableCompiledTilesetTableCount)
                return FALSE;
            s_ts_ov_secondary[i] = gFireredPortableCompiledTilesetTable[si];
        }
    }

    s_tileset_indices_rom_active = 1;
    return TRUE;
}

static u32 metatile_attr_word_count(const struct Tileset *ts)
{
    if (ts == NULL)
        return 0u;
    /* Compiled tileset row counts (640 + 384), not map-grid encoded secondary span (Phase 2). */
    return ts->isSecondary ? (u32)(NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY) : (u32)NUM_METATILES_IN_PRIMARY;
}

static u32 compiled_tileset_table_index_of(const struct Tileset *ts)
{
    u32 i;

    if (ts == NULL)
        return (u32)-1;
    for (i = 0u; i < gFireredPortableCompiledTilesetTableCount && i < FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX; i++)
    {
        if (gFireredPortableCompiledTilesetTable[i] == ts)
            return i;
    }
    return (u32)-1;
}

static size_t vanilla_metatile_gfx_bytes(const struct Tileset *ts)
{
    return (size_t)metatile_attr_word_count(ts) * (size_t)NUM_TILES_PER_METATILE * sizeof(u16);
}

static size_t vanilla_metatile_attr_bytes(const struct Tileset *ts)
{
    return (size_t)metatile_attr_word_count(ts) * sizeof(u32);
}

static size_t expected_metatile_gfx_bytes_for_tileset(const struct Tileset *ts)
{
    u32 idx;
    size_t van;

    if (ts == NULL)
        return 0u;
    van = vanilla_metatile_gfx_bytes(ts);
    if (s_compiled_ts_asset_profile_active == 0)
        return van;
    idx = compiled_tileset_table_index_of(ts);
    if (idx == (u32)-1)
        return van;
    if (s_compiled_ts_prof_metatile_gfx_bytes[idx] != 0u)
        return (size_t)s_compiled_ts_prof_metatile_gfx_bytes[idx];
    return van;
}

static size_t expected_metatile_attr_bytes_for_tileset(const struct Tileset *ts)
{
    u32 idx;
    size_t van;

    if (ts == NULL)
        return 0u;
    van = vanilla_metatile_attr_bytes(ts);
    if (s_compiled_ts_asset_profile_active == 0)
        return van;
    idx = compiled_tileset_table_index_of(ts);
    if (idx == (u32)-1)
        return van;
    if (s_compiled_ts_prof_metatile_attr_bytes[idx] != 0u)
        return (size_t)s_compiled_ts_prof_metatile_attr_bytes[idx];
    return van;
}

static bool8 try_apply_metatile_attributes_from_rom(size_t rom_size, const u8 *rom, size_t attr_off, u32 slot_count)
{
    u32 i;

    memset(s_layout_pri_metatile_attr, 0, sizeof(s_layout_pri_metatile_attr));
    memset(s_layout_sec_metatile_attr, 0, sizeof(s_layout_sec_metatile_attr));

    if (attr_off > rom_size || (size_t)slot_count * 8u > rom_size - attr_off)
        return FALSE;

    for (i = 0u; i < slot_count; i++)
    {
        const u8 *row = rom + attr_off + i * 8u;
        u32 po = read_le_u32(row + 0u);
        u32 so = read_le_u32(row + 4u);
        const struct MapLayout *ml = gMapLayouts[i];
        const struct Tileset *pri_ts;
        const struct Tileset *sec_ts;
        if (ml == NULL)
        {
            if (po != 0u || so != 0u)
                return FALSE;
            continue;
        }

        pri_ts = (s_tileset_indices_rom_active != 0 && s_ts_ov_primary[i] != NULL) ? s_ts_ov_primary[i] : ml->primaryTileset;
        sec_ts = (s_tileset_indices_rom_active != 0 && s_ts_ov_secondary[i] != NULL) ? s_ts_ov_secondary[i] : ml->secondaryTileset;

        if (po != 0u)
        {
            size_t nbytes = expected_metatile_attr_bytes_for_tileset(pri_ts);

            if (pri_ts == NULL || nbytes == 0u || (po & 3u) != 0u || po > rom_size || nbytes > rom_size - po)
                return FALSE;
            s_layout_pri_metatile_attr[i] = (const u32 *)(const void *)(rom + po);
        }

        if (so != 0u)
        {
            size_t nbytes = expected_metatile_attr_bytes_for_tileset(sec_ts);

            if (sec_ts == NULL || nbytes == 0u || (so & 3u) != 0u || so > rom_size || nbytes > rom_size - so)
                return FALSE;
            s_layout_sec_metatile_attr[i] = (const u32 *)(const void *)(rom + so);
        }
    }

    s_metatile_attributes_rom_active = 1;
    return TRUE;
}

static bool8 try_apply_metatile_graphics_from_rom(size_t rom_size, const u8 *rom, size_t gfx_off, u32 slot_count)
{
    u32 i;

    memset(s_layout_pri_metatile_gfx, 0, sizeof(s_layout_pri_metatile_gfx));
    memset(s_layout_sec_metatile_gfx, 0, sizeof(s_layout_sec_metatile_gfx));

    if (gfx_off > rom_size || (size_t)slot_count * 8u > rom_size - gfx_off)
        return FALSE;

    for (i = 0u; i < slot_count; i++)
    {
        const u8 *row = rom + gfx_off + i * 8u;
        u32 po = read_le_u32(row + 0u);
        u32 so = read_le_u32(row + 4u);
        const struct MapLayout *ml = gMapLayouts[i];
        const struct Tileset *pri_ts;
        const struct Tileset *sec_ts;
        size_t pri_bytes;
        size_t sec_bytes;

        if (ml == NULL)
        {
            if (po != 0u || so != 0u)
                return FALSE;
            continue;
        }

        pri_ts = (s_tileset_indices_rom_active != 0 && s_ts_ov_primary[i] != NULL) ? s_ts_ov_primary[i] : ml->primaryTileset;
        sec_ts = (s_tileset_indices_rom_active != 0 && s_ts_ov_secondary[i] != NULL) ? s_ts_ov_secondary[i] : ml->secondaryTileset;
        pri_bytes = expected_metatile_gfx_bytes_for_tileset(pri_ts);
        sec_bytes = expected_metatile_gfx_bytes_for_tileset(sec_ts);

        if (po != 0u)
        {
            if (pri_ts == NULL || pri_bytes == 0u || (po & 1u) != 0u || po > rom_size || pri_bytes > rom_size - po)
                return FALSE;
            s_layout_pri_metatile_gfx[i] = (const u16 *)(const void *)(rom + po);
        }

        if (so != 0u)
        {
            if (sec_ts == NULL || sec_bytes == 0u || (so & 1u) != 0u || so > rom_size || sec_bytes > rom_size - so)
                return FALSE;
            s_layout_sec_metatile_gfx[i] = (const u16 *)(const void *)(rom + so);
        }
    }

    s_metatile_graphics_rom_active = 1;
    return TRUE;
}

static size_t compiled_tileset_expected_tile_bytes(u32 table_index, const struct Tileset *ts)
{
    size_t pri_b;
    size_t sec_b;
    size_t vanilla;

    pri_b = (size_t)NUM_TILES_IN_PRIMARY * 32u;
    sec_b = (size_t)(NUM_TILES_TOTAL - NUM_TILES_IN_PRIMARY) * 32u;
    vanilla = (ts->isSecondary != FALSE) ? sec_b : pri_b;

    if (s_compiled_ts_asset_profile_active != 0 && table_index < FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX
        && table_index < gFireredPortableCompiledTilesetTableCount)
    {
        u32 o = s_compiled_ts_prof_tile_uncomp[table_index];

        if (o != 0u)
            return (size_t)o;
    }
    return vanilla;
}

static size_t compiled_tileset_expected_palette_bytes(u32 table_index)
{
    size_t vanilla;

    vanilla = (size_t)NUM_PALS_TOTAL * (size_t)PLTT_SIZE_4BPP;

    if (s_compiled_ts_asset_profile_active == 0 || table_index >= FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX
        || table_index >= gFireredPortableCompiledTilesetTableCount)
        return vanilla;

    if (s_compiled_ts_prof_palette_bundle_bytes[table_index] != 0u)
        return (size_t)s_compiled_ts_prof_palette_bundle_bytes[table_index];
    return vanilla;
}

static bool8 try_apply_compiled_tileset_asset_profile_from_rom(size_t rom_size, const u8 *rom, size_t prof_off)
{
    u32 i;
    u32 n;
    size_t vanilla_palette_blob;
    size_t tail;
    u32 row_stride;
    u32 gfx_unit;

    memset(s_compiled_ts_prof_tile_uncomp, 0, sizeof(s_compiled_ts_prof_tile_uncomp));
    memset(s_compiled_ts_prof_palette_bundle_bytes, 0, sizeof(s_compiled_ts_prof_palette_bundle_bytes));
    memset(s_compiled_ts_prof_metatile_gfx_bytes, 0, sizeof(s_compiled_ts_prof_metatile_gfx_bytes));
    memset(s_compiled_ts_prof_metatile_attr_bytes, 0, sizeof(s_compiled_ts_prof_metatile_attr_bytes));

    vanilla_palette_blob = (size_t)NUM_PALS_TOTAL * (size_t)PLTT_SIZE_4BPP;
    gfx_unit = (u32)NUM_TILES_PER_METATILE * (u32)sizeof(u16);

    n = gFireredPortableCompiledTilesetTableCount;
    if (n == 0u || n > FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX)
        return FALSE;

    if (prof_off > rom_size)
        return FALSE;
    tail = rom_size - prof_off;
    if (tail < (size_t)n * 8u)
        return FALSE;
    if (tail >= (size_t)n * sizeof(CompiledTilesetAssetProfileWire))
        row_stride = (u32)sizeof(CompiledTilesetAssetProfileWire);
    else
        row_stride = 8u;

    for (i = 0u; i < n; i++)
    {
        const u8 *row = rom + prof_off + (size_t)i * (size_t)row_stride;
        u32 tile_u = read_le_u32(row + 0u);
        u32 pal_b = read_le_u32(row + 4u);
        u32 gfx_b = 0u;
        u32 attr_b = 0u;
        const struct Tileset *ts = gFireredPortableCompiledTilesetTable[i];
        size_t van_gfx;
        size_t van_attr;

        if (row_stride >= 16u)
        {
            gfx_b = read_le_u32(row + 8u);
            attr_b = read_le_u32(row + 12u);
        }

        if (ts == NULL)
        {
            if (tile_u != 0u || pal_b != 0u || gfx_b != 0u || attr_b != 0u)
                return FALSE;
            continue;
        }

        van_gfx = vanilla_metatile_gfx_bytes(ts);
        van_attr = vanilla_metatile_attr_bytes(ts);

        if (pal_b != 0u)
        {
            if ((pal_b % (u32)PLTT_SIZE_4BPP) != 0u || (size_t)pal_b < vanilla_palette_blob
                || pal_b > FIRERED_PORTABLE_COMPILED_TILESET_PROFILE_PAL_BUNDLE_MAX)
                return FALSE;
        }
        if (tile_u != 0u)
        {
            if ((tile_u % 32u) != 0u || tile_u < 32u || tile_u > FIRERED_PORTABLE_COMPILED_TILESET_PROFILE_TILE_UNCOMP_MAX)
                return FALSE;
            /* LoadBgTiles() size is u16; uncompressed VRAM uploads cannot exceed that. */
            if (ts->isCompressed == 0 && tile_u > 65535u)
                return FALSE;
        }
        if (gfx_b != 0u)
        {
            if ((gfx_b % gfx_unit) != 0u || (size_t)gfx_b < van_gfx || gfx_b > FIRERED_PORTABLE_COMPILED_TILESET_PROFILE_METATILE_GFX_MAX)
                return FALSE;
        }
        if (attr_b != 0u)
        {
            if ((attr_b % (u32)sizeof(u32)) != 0u || (size_t)attr_b < van_attr
                || attr_b > FIRERED_PORTABLE_COMPILED_TILESET_PROFILE_METATILE_ATTR_MAX)
                return FALSE;
        }

        s_compiled_ts_prof_tile_uncomp[i] = tile_u;
        s_compiled_ts_prof_palette_bundle_bytes[i] = pal_b;
        s_compiled_ts_prof_metatile_gfx_bytes[i] = gfx_b;
        s_compiled_ts_prof_metatile_attr_bytes[i] = attr_b;
    }

    s_compiled_ts_asset_profile_active = 1;
    return TRUE;
}

static bool8 rom_lz77_tile_blob_header_ok(size_t rom_size, const u8 *rom, u32 o, size_t expect_uncomp)
{
    u32 uncomp;

    if ((o & 3u) != 0u || (size_t)o + 4u > rom_size)
        return FALSE;
    if (rom[o] != 0x10)
        return FALSE;
    uncomp = (u32)rom[o + 1] | ((u32)rom[o + 2] << 8) | ((u32)rom[o + 3] << 16);
    if (uncomp != (u32)expect_uncomp)
        return FALSE;
    return TRUE;
}

static bool8 try_apply_compiled_tileset_tiles_from_rom(size_t rom_size, const u8 *rom, size_t ct_off)
{
    u32 i;
    u32 n;
    u8 any_set;

    memset(s_compiled_ts_tile_data, 0, sizeof(s_compiled_ts_tile_data));
    any_set = 0;

    n = gFireredPortableCompiledTilesetTableCount;
    if (n == 0u || n > FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX)
        return FALSE;
    if (ct_off > rom_size || (size_t)n * 4u > rom_size - ct_off)
        return FALSE;

    for (i = 0u; i < n; i++)
    {
        u32 o = read_le_u32(rom + ct_off + i * 4u);
        const struct Tileset *ts = gFireredPortableCompiledTilesetTable[i];
        size_t need;

        if (o == 0u)
            continue;
        if (ts == NULL)
            return FALSE;

        need = compiled_tileset_expected_tile_bytes(i, ts);

        if ((o & 3u) != 0u || o > rom_size)
            return FALSE;

        if (ts->isCompressed != 0)
        {
            if (!rom_lz77_tile_blob_header_ok(rom_size, rom, o, need))
                return FALSE;
        }
        else
        {
            if (need > rom_size - o)
                return FALSE;
        }

        s_compiled_ts_tile_data[i] = (const u32 *)(const void *)(rom + o);
        any_set = 1;
    }

    if (any_set != 0)
        s_compiled_ts_tiles_rom_active = 1;
    return TRUE;
}

static bool8 try_apply_compiled_tileset_palettes_from_rom(size_t rom_size, const u8 *rom, size_t cp_off)
{
    u32 i;
    u32 n;
    size_t blob_b;
    u8 any_set;

    memset(s_compiled_ts_pal_data, 0, sizeof(s_compiled_ts_pal_data));
    any_set = 0;

    n = gFireredPortableCompiledTilesetTableCount;
    if (n == 0u || n > FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX)
        return FALSE;
    if (cp_off > rom_size || (size_t)n * 4u > rom_size - cp_off)
        return FALSE;

    for (i = 0u; i < n; i++)
    {
        u32 o = read_le_u32(rom + cp_off + i * 4u);
        const struct Tileset *ts = gFireredPortableCompiledTilesetTable[i];

        if (o == 0u)
            continue;
        if (ts == NULL)
            return FALSE;
        /*
         * ROM blob matches the uncompressed LoadPalette paths in LoadTilesetPalette (primary / secondary).
         * Tilesets that use LoadCompressedPalette for palettes need compiled data; reject non-zero rows.
         */
        if (ts->isSecondary != FALSE && ts->isSecondary != TRUE)
            return FALSE;
        blob_b = compiled_tileset_expected_palette_bytes(i);
        if ((o & 1u) != 0u || o > rom_size || blob_b > rom_size - o)
            return FALSE;

        s_compiled_ts_pal_data[i] = (const u16 *)(const void *)(rom + o);
        any_set = 1;
    }

    if (any_set != 0)
        s_compiled_ts_pals_rom_active = 1;
    return TRUE;
}

void firered_portable_rom_map_layout_metatiles_refresh_after_rom_load(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t dir_off;
    size_t dim_off;
    size_t ts_off;
    size_t attr_dir_off;
    size_t gfx_dir_off;
    size_t compiled_ts_tiles_off;
    size_t compiled_ts_palettes_off;
    u32 slot_count;
    size_t dir_bytes;
    u32 i;
    u8 have_dim_pack;
    u8 have_meta_dir;
    u8 have_ts_dir;
    u8 have_attr_dir;
    u8 have_gfx_dir;
    u8 have_comp_ts_tiles_dir;
    u8 have_comp_ts_pals_dir;
    u8 have_comp_ts_asset_prof_dir;
    size_t compiled_ts_asset_profile_off;

    clear_metatile_ptrs();
    clear_dim_rom_state();
    clear_tileset_overrides();
    clear_metatile_attributes_overlay();
    clear_metatile_graphics_overlay();
    clear_compiled_tileset_tiles_overlay();
    clear_compiled_tileset_palettes_overlay();
    clear_compiled_tileset_asset_profile();
    clear_field_map_palette_policy();
    rebuild_effective_layouts_compiled_only();

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom_size >= FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES && rom != NULL)
        try_apply_field_map_palette_policy_from_rom(rom_size, rom);

    slot_count = gMapLayoutsSlotCount;
    if (slot_count == 0u || slot_count > FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS)
        return;

    if (rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES || rom == NULL)
        return;

    have_meta_dir = (u8)(firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF",
        s_map_layout_metatiles_profile_rows, NELEMS(s_map_layout_metatiles_profile_rows), &dir_off) != 0);
    have_ts_dir = (u8)(firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF",
        s_map_layout_tileset_indices_profile_rows, NELEMS(s_map_layout_tileset_indices_profile_rows), &ts_off) != 0);
    have_attr_dir = (u8)(firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_LAYOUT_METATILE_ATTRIBUTES_DIRECTORY_OFF",
        s_map_layout_metatile_attributes_profile_rows, NELEMS(s_map_layout_metatile_attributes_profile_rows), &attr_dir_off) != 0);
    have_gfx_dir = (u8)(firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_LAYOUT_METATILE_GRAPHICS_DIRECTORY_OFF",
        s_map_layout_metatile_graphics_profile_rows, NELEMS(s_map_layout_metatile_graphics_profile_rows), &gfx_dir_off) != 0);
    have_comp_ts_tiles_dir = (u8)(firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_COMPILED_TILESET_TILES_DIRECTORY_OFF",
        s_compiled_tileset_tiles_profile_rows, NELEMS(s_compiled_tileset_tiles_profile_rows), &compiled_ts_tiles_off) != 0);
    have_comp_ts_pals_dir = (u8)(firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_COMPILED_TILESET_PALETTES_DIRECTORY_OFF",
        s_compiled_tileset_palettes_profile_rows, NELEMS(s_compiled_tileset_palettes_profile_rows), &compiled_ts_palettes_off) != 0);
    have_comp_ts_asset_prof_dir = (u8)(firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_COMPILED_TILESET_ASSET_PROFILE_OFF",
        s_compiled_tileset_asset_profile_profile_rows, NELEMS(s_compiled_tileset_asset_profile_profile_rows), &compiled_ts_asset_profile_off) != 0);

    if (have_meta_dir == 0 && have_ts_dir == 0 && have_attr_dir == 0 && have_gfx_dir == 0 && have_comp_ts_tiles_dir == 0
        && have_comp_ts_pals_dir == 0 && have_comp_ts_asset_prof_dir == 0)
        return;

    if (have_comp_ts_asset_prof_dir != 0)
    {
        if (!try_apply_compiled_tileset_asset_profile_from_rom(rom_size, rom, compiled_ts_asset_profile_off))
            clear_compiled_tileset_asset_profile();
    }

    if (have_meta_dir != 0)
    {
        dir_bytes = (size_t)slot_count * sizeof(MapLayoutMetatileDirEntryWire);
        if (dir_off > rom_size || dir_bytes > rom_size - dir_off)
        {
            clear_metatile_ptrs();
            clear_dim_rom_state();
        }
        else
        {
            have_dim_pack = (u8)(firered_portable_rom_u32_table_resolve_base_off("FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF",
                s_map_layout_dimensions_profile_rows, NELEMS(s_map_layout_dimensions_profile_rows), &dim_off) != 0);

            if (have_dim_pack != 0)
            {
                if (!parse_dimensions_directory(rom_size, rom, dim_off, slot_count))
                    clear_dim_rom_state();
            }

            for (i = 0u; i < (size_t)slot_count; i++)
            {
                const u8 *row = rom + dir_off + i * sizeof(MapLayoutMetatileDirEntryWire);
                u32 bo = read_le_u32(row + 0u);
                u32 mo = read_le_u32(row + 4u);
                u32 bw = read_le_u32(row + 8u);
                u32 mw = read_le_u32(row + 12u);
                const struct MapLayout *ml = gMapLayouts[i];
                u32 expect_bw;
                u32 expect_mw;
                s32 eff_w;
                s32 eff_h;
                u32 eff_bw;
                u32 eff_bh;
                size_t border_bytes;
                size_t map_bytes;

                if (ml == NULL)
                {
                    if (bo != 0u || mo != 0u || bw != 0u || mw != 0u)
                        goto meta_fail;
                    continue;
                }

                if (s_dim_rom_active[i] != 0)
                {
                    eff_w = (s32)s_dim_rom_w[i];
                    eff_h = (s32)s_dim_rom_h[i];
                    eff_bw = (u32)s_dim_rom_bw[i];
                    eff_bh = (u32)s_dim_rom_bh[i];
                }
                else
                {
                    eff_w = ml->width;
                    eff_h = ml->height;
                    eff_bw = (u32)ml->borderWidth;
                    eff_bh = (u32)ml->borderHeight;
                }

                if (eff_w <= 0 || eff_h <= 0)
                    goto meta_fail;

                expect_bw = eff_bw * eff_bh;
                expect_mw = (u32)eff_w * (u32)eff_h;

                if (bw != expect_bw || mw != expect_mw)
                    goto meta_fail;

                border_bytes = (size_t)bw * 2u;
                map_bytes = (size_t)mw * 2u;

                if ((bo | mo) & 1u)
                    goto meta_fail;
                if (bo > rom_size || border_bytes > rom_size - bo)
                    goto meta_fail;
                if (mo > rom_size || map_bytes > rom_size - mo)
                    goto meta_fail;

                s_border_ptr[i] = (const u16 *)(const void *)(rom + bo);
                s_map_ptr[i] = (const u16 *)(const void *)(rom + mo);
            }

            s_map_layout_metatiles_rom_active = 1;
            goto meta_done;

        meta_fail:
            clear_metatile_ptrs();
            clear_dim_rom_state();
        }
    }

meta_done:

    if (have_ts_dir != 0)
    {
        if (!try_apply_tileset_indices_from_rom(rom_size, rom, ts_off, slot_count))
            clear_tileset_overrides();
    }

    if (have_attr_dir != 0)
    {
        if (!try_apply_metatile_attributes_from_rom(rom_size, rom, attr_dir_off, slot_count))
            clear_metatile_attributes_overlay();
    }

    if (have_gfx_dir != 0)
    {
        if (!try_apply_metatile_graphics_from_rom(rom_size, rom, gfx_dir_off, slot_count))
            clear_metatile_graphics_overlay();
    }

    if (have_comp_ts_tiles_dir != 0)
    {
        if (!try_apply_compiled_tileset_tiles_from_rom(rom_size, rom, compiled_ts_tiles_off))
            clear_compiled_tileset_tiles_overlay();
    }

    if (have_comp_ts_pals_dir != 0)
    {
        if (!try_apply_compiled_tileset_palettes_from_rom(rom_size, rom, compiled_ts_palettes_off))
            clear_compiled_tileset_palettes_overlay();
    }

    rebuild_effective_layouts_final();
}

bool8 FireredRomMapLayoutMetatilesRomActive(void)
{
    return (bool8)(s_map_layout_metatiles_rom_active != 0);
}

bool8 FireredRomMapLayoutTilesetIndicesRomActive(void)
{
    return (bool8)(s_tileset_indices_rom_active != 0);
}

bool8 FireredRomMapLayoutMetatileAttributesRomActive(void)
{
    return (bool8)(s_metatile_attributes_rom_active != 0);
}

const u32 *FireredMapLayoutMetatileAttributesPtrForPrimary(const struct MapLayout *layout)
{
    int slot;

    if (layout == NULL || layout->primaryTileset == NULL)
        return NULL;
    if (s_metatile_attributes_rom_active == 0)
        return layout->primaryTileset->metatileAttributes;

    slot = layout_ptr_to_slot(layout);
    if (slot >= 0 && slot < (int)FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS && s_layout_pri_metatile_attr[slot] != NULL)
        return s_layout_pri_metatile_attr[slot];
    return layout->primaryTileset->metatileAttributes;
}

const u32 *FireredMapLayoutMetatileAttributesPtrForSecondary(const struct MapLayout *layout)
{
    int slot;

    if (layout == NULL || layout->secondaryTileset == NULL)
        return NULL;
    if (s_metatile_attributes_rom_active == 0)
        return layout->secondaryTileset->metatileAttributes;

    slot = layout_ptr_to_slot(layout);
    if (slot >= 0 && slot < (int)FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS && s_layout_sec_metatile_attr[slot] != NULL)
        return s_layout_sec_metatile_attr[slot];
    return layout->secondaryTileset->metatileAttributes;
}

bool8 FireredRomMapLayoutMetatileGraphicsRomActive(void)
{
    return (bool8)(s_metatile_graphics_rom_active != 0);
}

const u16 *FireredMapLayoutMetatileGraphicsPtrForPrimary(const struct MapLayout *layout)
{
    int slot;

    if (layout == NULL || layout->primaryTileset == NULL)
        return NULL;
    if (s_metatile_graphics_rom_active == 0)
        return layout->primaryTileset->metatiles;

    slot = layout_ptr_to_slot(layout);
    if (slot >= 0 && slot < (int)FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS && s_layout_pri_metatile_gfx[slot] != NULL)
        return s_layout_pri_metatile_gfx[slot];
    return layout->primaryTileset->metatiles;
}

const u16 *FireredMapLayoutMetatileGraphicsPtrForSecondary(const struct MapLayout *layout)
{
    int slot;

    if (layout == NULL || layout->secondaryTileset == NULL)
        return NULL;
    if (s_metatile_graphics_rom_active == 0)
        return layout->secondaryTileset->metatiles;

    slot = layout_ptr_to_slot(layout);
    if (slot >= 0 && slot < (int)FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS && s_layout_sec_metatile_gfx[slot] != NULL)
        return s_layout_sec_metatile_gfx[slot];
    return layout->secondaryTileset->metatiles;
}

bool8 FireredRomCompiledTilesetTilesRomActive(void)
{
    return (bool8)(s_compiled_ts_tiles_rom_active != 0);
}

bool8 FireredRomCompiledTilesetAssetProfileRomActive(void)
{
    return (bool8)(s_compiled_ts_asset_profile_active != 0);
}

u32 FireredPortableCompiledTilesetVramTileBytesForLoad(const struct Tileset *tileset, u16 numTilesVanilla)
{
    u32 i;
    u32 vanilla;

    vanilla = (u32)numTilesVanilla * 32u;
    if (tileset == NULL)
        return vanilla;

    if (s_compiled_ts_asset_profile_active == 0)
        return vanilla;

    for (i = 0u; i < gFireredPortableCompiledTilesetTableCount && i < FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX; i++)
    {
        if (gFireredPortableCompiledTilesetTable[i] != tileset)
            continue;
        if (s_compiled_ts_prof_tile_uncomp[i] != 0u)
            return s_compiled_ts_prof_tile_uncomp[i];
        return vanilla;
    }
    return vanilla;
}

const u32 *FireredPortableCompiledTilesetTilesForLoad(const struct Tileset *tileset)
{
    u32 i;

    if (tileset == NULL)
        return NULL;
    if (s_compiled_ts_tiles_rom_active == 0)
        return tileset->tiles;

    for (i = 0u; i < gFireredPortableCompiledTilesetTableCount && i < FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX; i++)
    {
        if (gFireredPortableCompiledTilesetTable[i] != tileset)
            continue;
        if (s_compiled_ts_tile_data[i] != NULL)
            return s_compiled_ts_tile_data[i];
        break;
    }
    return tileset->tiles;
}

bool8 FireredRomCompiledTilesetPalettesRomActive(void)
{
    return (bool8)(s_compiled_ts_pals_rom_active != 0);
}

const u16 *FireredPortableCompiledTilesetPaletteRowPtr(const struct Tileset *tileset, u32 row)
{
    u32 i;
    const u16 *blob;
    u32 pal_u16_per_row;
    u32 max_row;

    if (tileset == NULL)
        return NULL;

    pal_u16_per_row = (u32)(PLTT_SIZE_4BPP / sizeof(u16));

    if (s_compiled_ts_pals_rom_active != 0)
    {
        for (i = 0u; i < gFireredPortableCompiledTilesetTableCount && i < FIRERED_PORTABLE_COMPILED_TILESET_TILES_MAX; i++)
        {
            if (gFireredPortableCompiledTilesetTable[i] != tileset)
                continue;
            blob = s_compiled_ts_pal_data[i];
            if (blob != NULL)
            {
                max_row = (u32)(compiled_tileset_expected_palette_bytes(i) / (size_t)PLTT_SIZE_4BPP);
                if (row >= max_row)
                    return NULL;
                return blob + (size_t)row * (size_t)pal_u16_per_row;
            }
            break;
        }
    }

    if (row >= (u32)NUM_PALS_TOTAL)
        return NULL;
    return tileset->palettes[row];
}

const u16 *FireredPortableCompiledTilesetPalettePtrForLoad(const struct Tileset *tileset)
{
    if (tileset == NULL)
        return NULL;
    if (tileset->isSecondary == FALSE)
        return FireredPortableCompiledTilesetPaletteRowPtr(tileset, 0u);
    if (tileset->isSecondary == TRUE)
        return FireredPortableCompiledTilesetPaletteRowPtr(tileset, (u32)FireredPortableFieldMapBgPalettePrimaryRows());
    return NULL;
}

const struct MapLayout *FireredPortableEffectiveMapLayoutForLayoutId(u16 mapLayoutId)
{
    unsigned slot;
    const struct MapLayout *ml;

    if (mapLayoutId == 0u || mapLayoutId > gMapLayoutsSlotCount)
        return NULL;
    slot = (unsigned)mapLayoutId - 1u;
    if (slot >= FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS)
        return NULL;
    ml = gMapLayouts[slot];
    if (ml == NULL)
        return NULL;
    if (s_effective_valid[slot] != 0)
        return &s_effective_layout[slot];
    return ml;
}

const u16 *FireredMapLayoutMetatileMapPtrForLayoutId(u16 mapLayoutId)
{
    unsigned slot;
    const struct MapLayout *ml;

    if (mapLayoutId == 0u || mapLayoutId > gMapLayoutsSlotCount)
        return NULL;
    slot = (unsigned)mapLayoutId - 1u;
    if (slot >= FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS)
        return NULL;
    ml = gMapLayouts[slot];
    if (ml == NULL)
        return NULL;

    if (!s_map_layout_metatiles_rom_active)
        return ml->map;
    if (s_map_ptr[slot] != NULL)
        return s_map_ptr[slot];
    return ml->map;
}

const u16 *FireredMapLayoutMetatileBorderPtrForLayoutId(u16 mapLayoutId)
{
    unsigned slot;
    const struct MapLayout *ml;

    if (mapLayoutId == 0u || mapLayoutId > gMapLayoutsSlotCount)
        return NULL;
    slot = (unsigned)mapLayoutId - 1u;
    if (slot >= FIRERED_ROM_MAP_LAYOUT_METATILES_MAX_SLOTS)
        return NULL;
    ml = gMapLayouts[slot];
    if (ml == NULL)
        return NULL;

    if (!s_map_layout_metatiles_rom_active)
        return ml->border;
    if (s_border_ptr[slot] != NULL)
        return s_border_ptr[slot];
    return ml->border;
}

const u16 *FireredMapLayoutMetatileMapPtr(const struct MapLayout *layout)
{
    int slot;

    if (layout == NULL)
        return NULL;
    slot = layout_ptr_to_slot(layout);
    if (slot < 0)
        return layout->map;
    return FireredMapLayoutMetatileMapPtrForLayoutId((u16)(slot + 1));
}

const u16 *FireredMapLayoutMetatileBorderPtr(const struct MapLayout *layout)
{
    int slot;

    if (layout == NULL)
        return NULL;
    slot = layout_ptr_to_slot(layout);
    if (slot < 0)
        return layout->border;
    return FireredMapLayoutMetatileBorderPtrForLayoutId((u16)(slot + 1));
}

#endif /* PORTABLE */
