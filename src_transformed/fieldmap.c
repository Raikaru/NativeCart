#include "global.h"
#include "gflib.h"
#include "overworld.h"
#include "script.h"
#include "new_menu_helpers.h"
#include "quest_log.h"
#include "fieldmap.h"
#include "map_header_scalars_access.h"
#include "map_layout_metatiles_access.h"

/* Project C: encoded metatile id span must match the Phase 2 RAM mask in map block words. */
STATIC_ASSERT((u32)(MAP_GRID_METATILE_ID_SPACE_SIZE - 1u) == (u32)MAPGRID_METATILE_ID_MASK,
              map_grid_encoded_space_matches_metatile_id_mask);
STATIC_ASSERT((u32)MAPGRID_METATILE_ID_MASK == (u32)MAP_GRID_BLOCK_WORD_METATILE_ID_MASK,
              map_grid_block_word_mask_matches_global_fieldmap);
STATIC_ASSERT((u32)MAPGRID_COLLISION_MASK == (u32)MAP_GRID_BLOCK_WORD_COLLISION_MASK,
              map_grid_block_word_collision_mask_matches);
STATIC_ASSERT((u32)MAPGRID_ELEVATION_MASK == (u32)MAP_GRID_BLOCK_WORD_ELEVATION_MASK,
              map_grid_block_word_elevation_mask_matches);
STATIC_ASSERT((u32)MAP_GRID_METATILE_ID_SPACE_SIZE == (u32)MAP_GRID_BLOCK_WORD_ENCODED_METATILE_COUNT,
              map_grid_block_word_encoded_count_matches_fieldmap);

#ifdef PORTABLE
extern void firered_runtime_trace_external(const char *message);
#endif

struct ConnectionFlags
{
    u8 south:1;
    u8 north:1;
    u8 west:1;
    u8 east:1;
};

COMMON_DATA struct BackupMapLayout VMap = {0};
/* Live overworld grid: `u16` block words per `global.fieldmap.h` / Project C (`project_c_map_block_word_and_tooling_boundary.md`). */
EWRAM_DATA u16 gBackupMapData[VIRTUAL_MAP_SIZE] = {};
EWRAM_DATA struct MapHeader gMapHeader = {};
EWRAM_DATA struct Camera gCamera = {};
static EWRAM_DATA struct ConnectionFlags gMapConnectionFlags = {};
EWRAM_DATA u8 gGlobalFieldTintMode = QL_TINT_NONE;

static const struct ConnectionFlags sDummyConnectionFlags = {};

static void InitMapLayoutData(struct MapHeader *);
static void InitBackupMapLayoutData(const u16 *, u16, u16);
static void InitBackupMapLayoutConnections(struct MapHeader *);
static void FillSouthConnection(struct MapHeader const *, struct MapHeader const *, const struct MapConnection *, s32);
static void FillNorthConnection(struct MapHeader const *, struct MapHeader const *, const struct MapConnection *, s32);
static void FillWestConnection(struct MapHeader const *, struct MapHeader const *, const struct MapConnection *, s32);
static void FillEastConnection(struct MapHeader const *, struct MapHeader const *, const struct MapConnection *, s32);
static void LoadSavedMapView(void);
static void NormalizeSavedMapViewBlockWords(void);
static const struct MapConnection *GetIncomingConnection(u8, s32, s32);
static bool8 IsPosInIncomingConnectingMap(u8, s32, s32, const struct MapConnection *);
static bool8 IsCoordInIncomingConnectingMap(s32, s32, s32, s32);
static u32 GetAttributeByMetatileIdAndMapLayout(const struct MapLayout *, u16, u8);

#ifdef PORTABLE
/*
 * Field tileset uploads target MAP_BG_FIELD_TILESET_GFX_BG with char bytes in
 * [0, MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES) from the BG char base (physical layout). Direction B:
 * metatile logical TIDs are mapped to physical TIDs in field_camera via
 * FieldMapTranslateMetatileTileHalfwordForFieldBg (identity today). When
 * FieldMapFieldTileUploadUsesIdentityLayout() is false, uploads scatter each source tile to
 * gFieldMapLogicalToPhysicalTileId[logical]; remapped compressed loads use synchronous DMA waits
 * before freeing the decompressed buffer. Uncompressed PORTABLE loads use LoadBgTiles(..., u16 size, ...);
 * compressed loads use u32-sized helpers. DMA into VRAM is also bounded in PORTABLE dma3_manager.c.
 */
static bool8 FieldPortableFieldTilesetBgUploadContractOk(u32 tileBytes, u16 offsetTiles, bool32 needsU16ByteLimit)
{
    if (tileBytes == 0u)
        return TRUE;
    if ((tileBytes % (u32)TILE_SIZE_4BPP) != 0u)
        return FALSE;
    if (needsU16ByteLimit && tileBytes > 0xFFFFu)
        return FALSE;
    if ((u32)offsetTiles * (u32)TILE_SIZE_4BPP + tileBytes > MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES)
        return FALSE;
    return TRUE;
}

static const struct MapLayout *FieldmapNeighborEffectiveLayout(const struct MapConnection *connection, const struct MapHeader *neighborHdr)
{
    u16 lid;
    const struct MapLayout *lay;

    if (neighborHdr == NULL)
        return NULL;
    lid = FireredRomMapHeaderScalarsEffectiveMapLayoutId(connection->mapGroup, connection->mapNum, neighborHdr->mapLayoutId);
    lay = FireredPortableEffectiveMapLayoutForLayoutId(lid);
    if (lay == NULL)
        lay = neighborHdr->mapLayout;
    return lay;
}

/* Current map: same effective layout identity as InitMapLayoutData / GetBorderBlockAtImpl (save mapLayoutId). */
static const struct MapLayout *FieldmapCurrentEffectiveLayout(void)
{
    const struct MapLayout *lay;

    lay = FireredPortableEffectiveMapLayoutForLayoutId(gSaveBlock1Ptr->mapLayoutId);
    if (lay == NULL)
        lay = gMapHeader.mapLayout;
    return lay;
}
#endif

#ifdef PORTABLE
static bool8 FieldPortableFieldSingleMappedTileDestOk(u16 destTileIndexTiles)
{
    return (u32)destTileIndexTiles * TILE_SIZE_4BPP + TILE_SIZE_4BPP <= MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES;
}
#endif

/* Border `border.bin` / ROM overlay: `u16` words matching live map grid layout (`global.fieldmap.h` / Project C). */
static u16 GetBorderBlockAtImpl(s32 x, s32 y)
{
    u16 block;
    s32 xprime;
    s32 yprime;
    const struct MapLayout *mapLayout;
    const u16 *borderSrc;
    u16 borderLid;

    borderLid = gSaveBlock1Ptr->mapLayoutId;
#ifdef PORTABLE
    mapLayout = FireredPortableEffectiveMapLayoutForLayoutId(borderLid);
    if (mapLayout == NULL)
        mapLayout = gMapHeader.mapLayout;
    borderSrc = FireredMapLayoutMetatileBorderPtrForLayoutId(borderLid);
    if (borderSrc == NULL)
        borderSrc = MAP_LAYOUT_METATILE_BORDER_PTR(mapLayout);
#else
    mapLayout = gMapHeader.mapLayout;
    borderSrc = MAP_LAYOUT_METATILE_BORDER_PTR(mapLayout);
#endif

    if (mapLayout->borderWidth == 0 || mapLayout->borderHeight == 0)
        return MapGridBlockWordApplyBorderWrapCollision(MapGridBlockWordFromWireV1(0));

    xprime = x - MAP_OFFSET;
    xprime += 8 * mapLayout->borderWidth;
    xprime %= mapLayout->borderWidth;

    yprime = y - MAP_OFFSET;
    yprime += 8 * mapLayout->borderHeight;
    yprime %= mapLayout->borderHeight;

    block = MapGridBlockWordApplyBorderWrapCollision(
        MapGridBlockWordFromWireV1(borderSrc[xprime + yprime * mapLayout->borderWidth]));
    return block;
}

#define GetBorderBlockAt(x, y) GetBorderBlockAtImpl((x), (y))

#define AreCoordsWithinMapGridBounds(x, y) (x >= 0 && x < VMap.Xsize && y >= 0 && y < VMap.Ysize)

/* Full `u16` word: interior from `gBackupMapData`, out-of-bounds from border wrap (`GetBorderBlockAt`). */
#define GetMapGridBlockAt(x, y) (AreCoordsWithinMapGridBounds(x, y) ? VMap.map[x + VMap.Xsize * y] : GetBorderBlockAt(x, y))

// Masks/shifts for metatile attributes
// This is the format of the data stored in each data/tilesets/*/*/metatile_attributes.bin file
static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR]       = 0x000001ff, // Bits 0-8
    [METATILE_ATTRIBUTE_TERRAIN]        = 0x00003e00, // Bits 9-13
    [METATILE_ATTRIBUTE_2]              = 0x0003c000, // Bits 14-17
    [METATILE_ATTRIBUTE_3]              = 0x00fc0000, // Bits 18-23
    [METATILE_ATTRIBUTE_ENCOUNTER_TYPE] = 0x07000000, // Bits 24-26
    [METATILE_ATTRIBUTE_5]              = 0x18000000, // Bits 27-28
    [METATILE_ATTRIBUTE_LAYER_TYPE]     = 0x60000000, // Bits 29-30
    [METATILE_ATTRIBUTE_7]              = 0x80000000  // Bit  31
};

static const u8 sMetatileAttrShifts[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR]       = 0,
    [METATILE_ATTRIBUTE_TERRAIN]        = 9,
    [METATILE_ATTRIBUTE_2]              = 14,
    [METATILE_ATTRIBUTE_3]              = 18,
    [METATILE_ATTRIBUTE_ENCOUNTER_TYPE] = 24,
    [METATILE_ATTRIBUTE_5]              = 27,
    [METATILE_ATTRIBUTE_LAYER_TYPE]     = 29,
    [METATILE_ATTRIBUTE_7]              = 31
};

const struct MapHeader * GetMapHeaderFromConnection(const struct MapConnection * connection)
{
    return Overworld_GetMapHeaderByGroupAndId(connection->mapGroup, connection->mapNum);
}

void InitMap(void)
{
    InitMapLayoutData(&gMapHeader);
    RunOnLoadMapScript();
}

void InitMapFromSavedGame(void)
{
    InitMapLayoutData(&gMapHeader);
    LoadSavedMapView();
    RunOnLoadMapScript();
}

/* Fills `gBackupMapData` from the layout metatile grid (`map.bin` or ROM overlay; same `u16` words as save `mapView`). */
static void InitMapLayoutData(struct MapHeader * mapHeader)
{
    const struct MapLayout * mapLayout = mapHeader->mapLayout;
    const u16 *mapTiles;
#ifdef PORTABLE
    /*
     * Match GetMapLayout / BindMapLayoutFromSave / GetBorderBlockAtImpl: metatile blob key is the
     * save-backed layout id, not necessarily mapHeader->mapLayoutId before bind ordering fixes.
     */
    mapTiles = FireredMapLayoutMetatileMapPtrForLayoutId(gSaveBlock1Ptr->mapLayoutId);
    if (mapTiles == NULL)
        mapTiles = MAP_LAYOUT_METATILE_MAP_PTR(mapLayout);
#else
    mapTiles = MAP_LAYOUT_METATILE_MAP_PTR(mapLayout);
#endif
    CpuFastFill16(MAPGRID_UNDEFINED, gBackupMapData, sizeof(gBackupMapData));
    VMap.map = gBackupMapData;
    VMap.Xsize = mapLayout->width + MAP_OFFSET_W;
    VMap.Ysize = mapLayout->height + MAP_OFFSET_H;
    AGB_ASSERT_EX(VMap.Xsize * VMap.Ysize <= VIRTUAL_MAP_SIZE, ABSPATH("fieldmap.c"), 158);
    InitBackupMapLayoutData(mapTiles, mapLayout->width, mapLayout->height);
    InitBackupMapLayoutConnections(mapHeader);
}

/* PRET wire V1 `map.bin` rows -> Phase 2 RAM (`MapGridBlockWordFromWireV1`). */
static void InitBackupMapLayoutData(const u16 *map, u16 width, u16 height)
{
    s32 y;
    u16 x;
    u16 *dest = VMap.map;
    dest += VMap.Xsize * 7 + MAP_OFFSET;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
            dest[x] = MapGridBlockWordFromWireV1(map[x]);
        dest += width + MAP_OFFSET_W;
        map += width;
    }
}

static void InitBackupMapLayoutConnections(struct MapHeader *mapHeader)
{
    s32 count;
    const struct MapConnection *connection;
    s32 i;

    gMapConnectionFlags = sDummyConnectionFlags;

    /*
     * This null pointer check is new to FireRed.  It was kept in
     * Emerald, with the above struct assignment moved to after
     * this check.
     */
    if (mapHeader->connections)
    {
        count = mapHeader->connections->count;
        connection = mapHeader->connections->connections;
        for (i = 0; i < count; i++, connection++)
        {
            struct MapHeader const *cMap = GetMapHeaderFromConnection(connection);
            u32 offset = connection->offset;
            switch (connection->direction)
            {
            case CONNECTION_SOUTH:
                FillSouthConnection(mapHeader, cMap, connection, offset);
                gMapConnectionFlags.south = TRUE;
                break;
            case CONNECTION_NORTH:
                FillNorthConnection(mapHeader, cMap, connection, offset);
                gMapConnectionFlags.north = TRUE;
                break;
            case CONNECTION_WEST:
                FillWestConnection(mapHeader, cMap, connection, offset);
                gMapConnectionFlags.west = TRUE;
                break;
            case CONNECTION_EAST:
                FillEastConnection(mapHeader, cMap, connection, offset);
                gMapConnectionFlags.east = TRUE;
                break;
            }
        }
    }
}

/* Copies connected-map PRET wire V1 words into `VMap` as Phase 2 RAM. */
static void FillConnection(s32 x, s32 y, const struct MapConnection *conn, const struct MapHeader *connectedMapHeader, s32 x2, s32 y2, s32 width, s32 height)
{
    s32 i;
    u16 j;
    const u16 *src;
    u16 *dest;
    s32 mapWidth;
    const struct MapLayout *dimLayout;
    const u16 *tileBase;

#ifdef PORTABLE
    {
        u16 lid = FireredRomMapHeaderScalarsEffectiveMapLayoutId(conn->mapGroup, conn->mapNum, connectedMapHeader->mapLayoutId);

        dimLayout = FireredPortableEffectiveMapLayoutForLayoutId(lid);
        if (dimLayout == NULL)
            dimLayout = connectedMapHeader->mapLayout;

        tileBase = FireredMapLayoutMetatileMapPtrForLayoutId(lid);
        if (tileBase == NULL)
            tileBase = MAP_LAYOUT_METATILE_MAP_PTR(dimLayout);
        mapWidth = dimLayout->width;
        src = &tileBase[mapWidth * y2 + x2];
    }
#else
    dimLayout = connectedMapHeader->mapLayout;
    (void)conn;
    mapWidth = dimLayout->width;
    src = &MAP_LAYOUT_METATILE_MAP_PTR(dimLayout)[mapWidth * y2 + x2];
#endif
    dest = &VMap.map[VMap.Xsize * y + x];

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < (u16)width; j++)
            dest[j] = MapGridBlockWordFromWireV1(src[j]);
        dest += VMap.Xsize;
        src += mapWidth;
    }
}

static void FillSouthConnection(struct MapHeader const *mapHeader, struct MapHeader const *connectedMapHeader, const struct MapConnection *conn, s32 offset)
{
    s32 x, y;
    s32 x2;
    s32 width;
    s32 cWidth;
    const struct MapLayout *cDim;

    if (connectedMapHeader)
    {
#ifdef PORTABLE
        {
            u16 lid = FireredRomMapHeaderScalarsEffectiveMapLayoutId(conn->mapGroup, conn->mapNum, connectedMapHeader->mapLayoutId);

            cDim = FireredPortableEffectiveMapLayoutForLayoutId(lid);
            if (cDim == NULL)
                cDim = connectedMapHeader->mapLayout;
        }
#else
        (void)conn;
        cDim = connectedMapHeader->mapLayout;
#endif
        cWidth = cDim->width;
        x = offset + MAP_OFFSET;
#ifdef PORTABLE
        y = FieldmapCurrentEffectiveLayout()->height + MAP_OFFSET;
#else
        y = mapHeader->mapLayout->height + MAP_OFFSET;
#endif
        if (x < 0)
        {
            x2 = -x;
            x += cWidth;
            if (x < VMap.Xsize)
                width = x;
            else
                width = VMap.Xsize;
            x = 0;
        }
        else
        {
            x2 = 0;
            if (x + cWidth < VMap.Xsize)
                width = cWidth;
            else
                width = VMap.Xsize - x;
        }

        FillConnection(
            x, y,
            conn, connectedMapHeader,
            x2, /*y2*/ 0,
            width, /*height*/ MAP_OFFSET);
    }
}

static void FillNorthConnection(struct MapHeader const *mapHeader, struct MapHeader const *connectedMapHeader, const struct MapConnection *conn, s32 offset)
{
    s32 x;
    s32 x2, y2;
    s32 width;
    s32 cWidth, cHeight;
    const struct MapLayout *cDim;

    if (connectedMapHeader)
    {
#ifdef PORTABLE
        {
            u16 lid = FireredRomMapHeaderScalarsEffectiveMapLayoutId(conn->mapGroup, conn->mapNum, connectedMapHeader->mapLayoutId);

            cDim = FireredPortableEffectiveMapLayoutForLayoutId(lid);
            if (cDim == NULL)
                cDim = connectedMapHeader->mapLayout;
        }
#else
        (void)conn;
        cDim = connectedMapHeader->mapLayout;
#endif
        cWidth = cDim->width;
        cHeight = cDim->height;
        x = offset + MAP_OFFSET;
        y2 = cHeight - MAP_OFFSET;
        if (x < 0)
        {
            x2 = -x;
            x += cWidth;
            if (x < VMap.Xsize)
                width = x;
            else
                width = VMap.Xsize;
            x = 0;
        }
        else
        {
            x2 = 0;
            if (x + cWidth < VMap.Xsize)
                width = cWidth;
            else
                width = VMap.Xsize - x;
        }

        FillConnection(
            x, /*y*/ 0,
            conn, connectedMapHeader,
            x2, y2,
            width, /*height*/ MAP_OFFSET);

    }
}

static void FillWestConnection(struct MapHeader const *mapHeader, struct MapHeader const *connectedMapHeader, const struct MapConnection *conn, s32 offset)
{
    s32 y;
    s32 x2, y2;
    s32 height;
    s32 cWidth, cHeight;
    const struct MapLayout *cDim;

    if (connectedMapHeader)
    {
#ifdef PORTABLE
        {
            u16 lid = FireredRomMapHeaderScalarsEffectiveMapLayoutId(conn->mapGroup, conn->mapNum, connectedMapHeader->mapLayoutId);

            cDim = FireredPortableEffectiveMapLayoutForLayoutId(lid);
            if (cDim == NULL)
                cDim = connectedMapHeader->mapLayout;
        }
#else
        (void)conn;
        cDim = connectedMapHeader->mapLayout;
#endif
        cWidth = cDim->width;
        cHeight = cDim->height;
        y = offset + MAP_OFFSET;
        x2 = cWidth - MAP_OFFSET;
        if (y < 0)
        {
            y2 = -y;
            if (y + cHeight < VMap.Ysize)
                height = y + cHeight;
            else
                height = VMap.Ysize;
            y = 0;
        }
        else
        {
            y2 = 0;
            if (y + cHeight < VMap.Ysize)
                height = cHeight;
            else
                height = VMap.Ysize - y;
        }

        FillConnection(
            /*x*/ 0, y,
            conn, connectedMapHeader,
            x2, y2,
            /*width*/ MAP_OFFSET, height);
    }
}

static void FillEastConnection(struct MapHeader const *mapHeader, struct MapHeader const *connectedMapHeader, const struct MapConnection *conn, s32 offset)
{
    s32 x, y;
    s32 y2;
    s32 height;
    s32 cHeight;
    const struct MapLayout *cDim;

    if (connectedMapHeader)
    {
#ifdef PORTABLE
        {
            u16 lid = FireredRomMapHeaderScalarsEffectiveMapLayoutId(conn->mapGroup, conn->mapNum, connectedMapHeader->mapLayoutId);

            cDim = FireredPortableEffectiveMapLayoutForLayoutId(lid);
            if (cDim == NULL)
                cDim = connectedMapHeader->mapLayout;
        }
#else
        (void)conn;
        cDim = connectedMapHeader->mapLayout;
#endif
        cHeight = cDim->height;
        x = mapHeader->mapLayout->width + MAP_OFFSET;
        y = offset + MAP_OFFSET;
        if (y < 0)
        {
            y2 = -y;
            if (y + cHeight < VMap.Ysize)
                height = y + cHeight;
            else
                height = VMap.Ysize;
            y = 0;
        }
        else
        {
            y2 = 0;
            if (y + cHeight < VMap.Ysize)
                height = cHeight;
            else
                height = VMap.Ysize - y;
        }

        FillConnection(
            x, y,
            conn, connectedMapHeader,
            /*x2*/ 0, y2,
            /*width*/ MAP_OFFSET + 1, height);
    }
}

u8 MapGridGetElevationAt(s32 x, s32 y)
{
    u16 block = GetMapGridBlockAt(x, y);

    return MapGridBlockWordGetElevation(block);
}

u8 MapGridGetCollisionAt(s32 x, s32 y)
{
    u16 block = GetMapGridBlockAt(x, y);

    return MapGridBlockWordGetCollision(block);
}

/*
 * Returns the metatile id in **encoded space** `0 .. MAP_GRID_METATILE_ID_SPACE_SIZE - 1` (Phase 2: 0..2047):
 * low **11 bits** of the Phase 2 RAM block word (`MAPGRID_METATILE_ID_MASK`). See
 * `docs/architecture/project_c_map_block_word_and_tooling_boundary.md`.
 */
u32 MapGridGetMetatileIdAt(s32 x, s32 y)
{
    u16 block = GetMapGridBlockAt(x, y);

    if (block == MAPGRID_UNDEFINED)
        return MapGridBlockWordGetMetatileIdFromWord(GetBorderBlockAt(x, y));

    return MapGridBlockWordGetMetatileIdFromWord(block);
}

u32 ExtractMetatileAttribute(u32 attributes, u8 attributeType)
{
    if (attributeType >= METATILE_ATTRIBUTE_COUNT) // Check for METATILE_ATTRIBUTES_ALL
        return attributes;

    return (attributes & sMetatileAttrMasks[attributeType]) >> sMetatileAttrShifts[attributeType];
}

u32 MapGridGetMetatileAttributeAt(s16 x, s16 y, u8 attributeType)
{
    u16 metatileId = MapGridGetMetatileIdAt(x, y);
#ifdef PORTABLE
    /* Same effective MapLayout as borders / metatile grid / connection geometry (save mapLayoutId). */
    return GetAttributeByMetatileIdAndMapLayout(FieldmapCurrentEffectiveLayout(), metatileId, attributeType);
#else
    return GetAttributeByMetatileIdAndMapLayout(gMapHeader.mapLayout, metatileId, attributeType);
#endif
}

u32 MapGridGetMetatileBehaviorAt(s16 x, s16 y)
{
    return MapGridGetMetatileAttributeAt(x, y, METATILE_ATTRIBUTE_BEHAVIOR);
}

u8 MapGridGetMetatileLayerTypeAt(s16 x, s16 y)
{
    return MapGridGetMetatileAttributeAt(x, y, METATILE_ATTRIBUTE_LAYER_TYPE);
}

/* Mutates metatile + collision bits; preserves elevation (`MAPGRID_*` in `global.fieldmap.h`). */
void MapGridSetMetatileIdAt(s32 x, s32 y, u16 metatile)
{
    s32 i;
    if (AreCoordsWithinMapGridBounds(x, y))
    {
        i = x + y * VMap.Xsize;
        VMap.map[i] = MapGridBlockWordReplaceMetatileAndCollisionKeepingElevation(VMap.map[i], metatile);
    }
}

/* Overwrites the full `u16` block word (must stay consistent with layout `map.bin` / save `mapView`). */
void MapGridSetMetatileEntryAt(s32 x, s32 y, u16 metatile)
{
    s32 i;
    if (AreCoordsWithinMapGridBounds(x, y))
    {
        i = x + VMap.Xsize * y;
        VMap.map[i] = metatile;
    }
}

/* Toggles collision bits only; metatile id and elevation unchanged. */
void MapGridSetMetatileImpassabilityAt(s32 x, s32 y, bool32 impassable)
{
    if (AreCoordsWithinMapGridBounds(x, y))
    {
        {
            s32 idx = x + VMap.Xsize * y;
            VMap.map[idx] = MapGridBlockWordSetImpassable(VMap.map[idx], impassable);
        }
    }
}

static u32 GetAttributeByMetatileIdAndMapLayout(const struct MapLayout *mapLayout, u16 metatile, u8 attributeType)
{
    const u32 * attributes;
    u32 mid = metatile;

    if (!MapGridMetatileIdIsInEncodedSpace(mid))
        return 0xFF;

    if (MapGridMetatileIdUsesPrimaryTileset(mid))
    {
#ifdef PORTABLE
        attributes = FireredMapLayoutMetatileAttributesPtrForPrimary(mapLayout);
#else
        attributes = mapLayout->primaryTileset->metatileAttributes;
#endif
        if (attributes == NULL)
            return 0xFF;
        return ExtractMetatileAttribute(attributes[mid], attributeType);
    }
    else if (MapGridMetatileIdUsesSecondaryTileset(mid))
    {
#ifdef PORTABLE
        attributes = FireredMapLayoutMetatileAttributesPtrForSecondary(mapLayout);
#else
        attributes = mapLayout->secondaryTileset->metatileAttributes;
#endif
        if (attributes == NULL)
            return 0xFF;
        return ExtractMetatileAttribute(attributes[MapGridMetatileSecondaryRowIndex(mid)], attributeType);
    }
    else
    {
        return 0xFF;
    }
}

/*
 * Persists a screen-sized window of `gBackupMapData` into `gSaveBlock2Ptr->mapView` (Fly / return UX).
 * Words are the same **`u16` block format** as the live map grid — any change to map block encoding
 * (Project C large program) must revisit save layout: `docs/architecture/project_c_map_block_word_and_tooling_boundary.md`.
 */
void SaveMapView(void)
{
    s32 i, j;
    s32 x, y;
    u16 *mapView;
    s32 width;
    mapView = gSaveBlock2Ptr->mapView;
    width = VMap.Xsize;
    x = gSaveBlock1Ptr->pos.x;
    y = gSaveBlock1Ptr->pos.y;
    for (i = y; i < y + MAP_OFFSET_H; i++)
    {
        for (j = x; j < x + MAP_OFFSET_W; j++)
            *mapView++ = gBackupMapData[width * i + j];
    }
    gSaveBlock1Ptr->mapGridBlockWordSaveLayout = MAP_GRID_BLOCK_WORD_LAYOUT_PHASE2_U16;
}

static bool32 SavedMapViewIsEmpty(void)
{
    u16 i;
    u32 marker = 0;

#ifndef UBFIX
    // BUG: This loop extends past the bounds of the mapView array. Its size is only 0x100.
    for (i = 0; i < 0x200; i++)
        marker |= gSaveBlock2Ptr->mapView[i];
#else
    for (i = 0; i < NELEMS(gSaveBlock2Ptr->mapView); i++)
        marker |= gSaveBlock2Ptr->mapView[i];
#endif

    if (marker == 0)
        return TRUE;
    else
        return FALSE;
}

static void ClearSavedMapView(void)
{
    CpuFill16(0, gSaveBlock2Ptr->mapView, sizeof(gSaveBlock2Ptr->mapView));
}

/*
 * Migrates `gSaveBlock2Ptr->mapView` from PRET **wire V1** to Phase 2 RAM words when needed.
 * `gSaveBlock1Ptr->mapGridBlockWordSaveLayout` == `MAP_GRID_BLOCK_WORD_LAYOUT_PHASE2_U16` => already Phase 2.
 */
static void NormalizeSavedMapViewBlockWords(void)
{
    u32 i;

    if (gSaveBlock1Ptr->mapGridBlockWordSaveLayout == MAP_GRID_BLOCK_WORD_LAYOUT_PHASE2_U16)
        return;

    for (i = 0; i < NELEMS(gSaveBlock2Ptr->mapView); i++)
        gSaveBlock2Ptr->mapView[i] = MapGridBlockWordFromWireV1(gSaveBlock2Ptr->mapView[i]);
    gSaveBlock1Ptr->mapGridBlockWordSaveLayout = MAP_GRID_BLOCK_WORD_LAYOUT_PHASE2_U16;
}

/* Inverse of `SaveMapView`: `mapView` words must stay bitwise-compatible with `gBackupMapData` / layout bins. */
static void LoadSavedMapView(void)
{
    s32 i, j;
    s32 x, y;
    u16 *mapView;
    s32 width;
    mapView = gSaveBlock2Ptr->mapView;
    if (!SavedMapViewIsEmpty())
    {
        NormalizeSavedMapViewBlockWords();
        width = VMap.Xsize;
        x = gSaveBlock1Ptr->pos.x;
        y = gSaveBlock1Ptr->pos.y;
        for (i = y; i < y + MAP_OFFSET_H; i++)
        {
            for (j = x; j < x + MAP_OFFSET_W; j++)
            {
                gBackupMapData[j + width * i] = *mapView;
                mapView++;
            }
        }
        ClearSavedMapView();
    }
}

/* Stitches `gSaveBlock2Ptr->mapView` into `gBackupMapData` when crossing a map connection (same `u16` word layout). */
static void MoveMapViewToBackup(u8 direction)
{
    s32 width;
    u16 *mapView;
    s32 x0, y0;
    s32 x2, y2;
    u16 *src, *dest;
    s32 srci, desti;
    s32 r9, r8;
    s32 x, y;
    s32 i, j;
    mapView = gSaveBlock2Ptr->mapView;
    width = VMap.Xsize;
    r9 = 0;
    r8 = 0;
    x0 = gSaveBlock1Ptr->pos.x;
    y0 = gSaveBlock1Ptr->pos.y;
    x2 = 15;
    y2 = 14;
    switch (direction)
    {
    case CONNECTION_NORTH:
        y0 += 1;
        y2 = MAP_OFFSET_H - 1;
        break;
    case CONNECTION_SOUTH:
        r8 = 1;
        y2 = MAP_OFFSET_H - 1;
        break;
    case CONNECTION_WEST:
        x0 += 1;
        x2 = MAP_OFFSET_W - 1;
        break;
    case CONNECTION_EAST:
        r9 = 1;
        x2 = MAP_OFFSET_W - 1;
        break;
    }
    for (y = 0; y < y2; y++)
    {
        i = 0;
        j = 0;
        for (x = 0; x < x2; x++)
        {
            desti = width * (y + y0);
            srci = (y + r8) * MAP_OFFSET_W + r9;
            src = &mapView[srci + i];
            dest = &gBackupMapData[x0 + desti + j];
            *dest = *src;
            i++;
            j++;
        }
    }
    ClearSavedMapView();
}

s32 GetMapBorderIdAt(s32 x, s32 y)
{
    if (GetMapGridBlockAt(x, y) == MAPGRID_UNDEFINED)
        return CONNECTION_INVALID;

    if (x >= VMap.Xsize - (MAP_OFFSET + 1))
    {
        if (!gMapConnectionFlags.east)
            return CONNECTION_INVALID;

        return CONNECTION_EAST;
    }

    if (x < MAP_OFFSET)
    {
        if (!gMapConnectionFlags.west)
            return CONNECTION_INVALID;

        return CONNECTION_WEST;
    }

    if (y >= VMap.Ysize - MAP_OFFSET)
    {
        if (!gMapConnectionFlags.south)
            return CONNECTION_INVALID;

        return CONNECTION_SOUTH;
    }

    if (y < MAP_OFFSET)
    {
        if (!gMapConnectionFlags.north)
            return CONNECTION_INVALID;

        return CONNECTION_NORTH;
    }

    return CONNECTION_NONE;
}

static s32 GetPostCameraMoveMapBorderId(s32 x, s32 y)
{
    return GetMapBorderIdAt(gSaveBlock1Ptr->pos.x + MAP_OFFSET + x, gSaveBlock1Ptr->pos.y + MAP_OFFSET + y);
}

bool32 CanCameraMoveInDirection(s32 direction)
{
    s32 x, y;
    x = gSaveBlock1Ptr->pos.x + MAP_OFFSET + gDirectionToVectors[direction].x;
    y = gSaveBlock1Ptr->pos.y + MAP_OFFSET + gDirectionToVectors[direction].y;

    if (GetMapBorderIdAt(x, y) == CONNECTION_INVALID)
        return FALSE;

    return TRUE;
}

static void SetPositionFromConnection(const struct MapConnection *connection, int direction, s32 x, s32 y)
{
    struct MapHeader const *mapHeader;
#ifdef PORTABLE
    const struct MapLayout *nLay;
#endif

    mapHeader = GetMapHeaderFromConnection(connection);
#ifdef PORTABLE
    nLay = FieldmapNeighborEffectiveLayout(connection, mapHeader);
#else
#define nLay (mapHeader->mapLayout)
#endif
    switch (direction)
    {
    case CONNECTION_EAST:
        gSaveBlock1Ptr->pos.x = -x;
        gSaveBlock1Ptr->pos.y -= connection->offset;
        break;
    case CONNECTION_WEST:
        gSaveBlock1Ptr->pos.x = nLay->width;
        gSaveBlock1Ptr->pos.y -= connection->offset;
        break;
    case CONNECTION_SOUTH:
        gSaveBlock1Ptr->pos.x -= connection->offset;
        gSaveBlock1Ptr->pos.y = -y;
        break;
    case CONNECTION_NORTH:
        gSaveBlock1Ptr->pos.x -= connection->offset;
        gSaveBlock1Ptr->pos.y = nLay->height;
        break;
    }
#ifndef PORTABLE
#undef nLay
#endif
}

bool8 CameraMove(s32 x, s32 y)
{
    s32 direction;
    const struct MapConnection *connection;
    s32 old_x, old_y;
    gCamera.active = FALSE;
    direction = GetPostCameraMoveMapBorderId(x, y);
    if (direction == CONNECTION_NONE || direction == CONNECTION_INVALID)
    {
        gSaveBlock1Ptr->pos.x += x;
        gSaveBlock1Ptr->pos.y += y;
    }
    else
    {
        SaveMapView();
        old_x = gSaveBlock1Ptr->pos.x;
        old_y = gSaveBlock1Ptr->pos.y;
        connection = GetIncomingConnection(direction, gSaveBlock1Ptr->pos.x, gSaveBlock1Ptr->pos.y);
        SetPositionFromConnection(connection, direction, x, y);
        LoadMapFromCameraTransition(connection->mapGroup, connection->mapNum);
        gCamera.active = TRUE;
        gCamera.x = old_x - gSaveBlock1Ptr->pos.x;
        gCamera.y = old_y - gSaveBlock1Ptr->pos.y;
        gSaveBlock1Ptr->pos.x += x;
        gSaveBlock1Ptr->pos.y += y;
        MoveMapViewToBackup(direction);
    }
    return gCamera.active;
}

const struct MapConnection *GetIncomingConnection(u8 direction, s32 x, s32 y)
{
    s32 count;
    const struct MapConnection *connection;
    const struct MapConnections *connections = gMapHeader.connections;
    s32 i;

#ifdef UBFIX // UB: Multiple possible null dereferences
    if (connections == NULL || connections->connections == NULL)
        return NULL;
#endif
    count = connections->count;
    connection = connections->connections;
    for (i = 0; i < count; i++, connection++)
    {
        if (connection->direction == direction && IsPosInIncomingConnectingMap(direction, x, y, connection) == TRUE)
            return connection;
    }
    return NULL;

}

static bool8 IsPosInIncomingConnectingMap(u8 direction, s32 x, s32 y, const struct MapConnection *connection)
{
    struct MapHeader const *mapHeader;
#ifdef PORTABLE
    const struct MapLayout *nLay;
#endif

    mapHeader = GetMapHeaderFromConnection(connection);
#ifdef PORTABLE
    nLay = FieldmapNeighborEffectiveLayout(connection, mapHeader);
#else
#define nLay (mapHeader->mapLayout)
#endif
    switch (direction)
    {
    case CONNECTION_SOUTH:
    case CONNECTION_NORTH:
#ifdef PORTABLE
        return IsCoordInIncomingConnectingMap(x, FieldmapCurrentEffectiveLayout()->width, nLay->width, connection->offset);
#else
        return IsCoordInIncomingConnectingMap(x, gMapHeader.mapLayout->width, nLay->width, connection->offset);
#endif
    case CONNECTION_WEST:
    case CONNECTION_EAST:
#ifdef PORTABLE
        return IsCoordInIncomingConnectingMap(y, FieldmapCurrentEffectiveLayout()->height, nLay->height, connection->offset);
#else
        return IsCoordInIncomingConnectingMap(y, gMapHeader.mapLayout->height, nLay->height, connection->offset);
#endif
    }
#ifndef PORTABLE
#undef nLay
#endif
    return FALSE;
}

static bool8 IsCoordInIncomingConnectingMap(s32 coord, s32 srcMax, s32 destMax, s32 offset)
{
    s32 offset2 = max(offset, 0);

    if (destMax + offset < srcMax)
        srcMax = destMax + offset;

    if (offset2 <= coord && coord <= srcMax)
        return TRUE;

    return FALSE;
}

static bool32 IsCoordInConnectingMap(s32 coord, s32 max)
{
    if (coord >= 0 && coord < max)
        return TRUE;

    return FALSE;
}

static s32 IsPosInConnectingMap(const struct MapConnection *connection, s32 x, s32 y)
{
    struct MapHeader const *mapHeader;
#ifdef PORTABLE
    const struct MapLayout *nLay;
#endif

    mapHeader = GetMapHeaderFromConnection(connection);
#ifdef PORTABLE
    nLay = FieldmapNeighborEffectiveLayout(connection, mapHeader);
#else
#define nLay (mapHeader->mapLayout)
#endif
    switch (connection->direction)
    {
    case CONNECTION_SOUTH:
    case CONNECTION_NORTH:
        return IsCoordInConnectingMap(x - connection->offset, nLay->width);
    case CONNECTION_WEST:
    case CONNECTION_EAST:
        return IsCoordInConnectingMap(y - connection->offset, nLay->height);
    }
#ifndef PORTABLE
#undef nLay
#endif
    return FALSE;
}

const struct MapConnection *GetMapConnectionAtPos(s16 x, s16 y)
{
    s32 count;
    const struct MapConnection *connection;
    s32 i;
    u8 direction;
    if (!gMapHeader.connections)
    {
        return NULL;
    }
    else
    {
#ifdef PORTABLE
        const struct MapLayout *curLay = FieldmapCurrentEffectiveLayout();
#endif
        count = gMapHeader.connections->count;
        connection = gMapHeader.connections->connections;
        for (i = 0; i < count; i++, connection++)
        {
            direction = connection->direction;
            if ((direction == CONNECTION_DIVE || direction == CONNECTION_EMERGE)
                || (direction == CONNECTION_NORTH && y > MAP_OFFSET - 1)
#ifdef PORTABLE
                || (direction == CONNECTION_SOUTH && y < curLay->height + MAP_OFFSET)
                || (direction == CONNECTION_WEST && x > MAP_OFFSET - 1)
                || (direction == CONNECTION_EAST && x < curLay->width + MAP_OFFSET))
#else
                || (direction == CONNECTION_SOUTH && y < gMapHeader.mapLayout->height + MAP_OFFSET)
                || (direction == CONNECTION_WEST && x > MAP_OFFSET - 1)
                || (direction == CONNECTION_EAST && x < gMapHeader.mapLayout->width + MAP_OFFSET))
#endif
            {
                continue;
            }

            if (IsPosInConnectingMap(connection, x - MAP_OFFSET, y - MAP_OFFSET) == TRUE)
                return connection;
        }
    }
    return NULL;
}

void SetCameraFocusCoords(u16 x, u16 y)
{
    gSaveBlock1Ptr->pos.x = x - MAP_OFFSET;
    gSaveBlock1Ptr->pos.y = y - MAP_OFFSET;
}

void GetCameraFocusCoords(u16 *x, u16 *y)
{
    *x = gSaveBlock1Ptr->pos.x + MAP_OFFSET;
    *y = gSaveBlock1Ptr->pos.y + MAP_OFFSET;
}

// Unused
static void SetCameraCoords(u16 x, u16 y)
{
    gSaveBlock1Ptr->pos.x = x;
    gSaveBlock1Ptr->pos.y = y;
}

void GetCameraCoords(u16 *x, u16 *y)
{
    *x = gSaveBlock1Ptr->pos.x;
    *y = gSaveBlock1Ptr->pos.y;
}

static void FieldMapSyncWaitForBgTilesDmaRequestIfValid(u16 loadBgTilesReturn)
{
    s16 idx = (s16)loadBgTilesReturn;

    if (idx < 0)
        return;
    while (WaitDma3Request(idx) != 0)
        ;
}

static void FieldMapCopyCompressedFieldTileSpanRemapped(
    const void *compressedSrc, u32 maxDecompressedTileBytes, u16 logicalFirstTileIndex)
{
    void *ptr;
    u32 sizeOut;
    u16 t;
    u16 tileCount;

    ptr = MallocAndDecompress(compressedSrc, &sizeOut);
    if (ptr == NULL)
        return;
    if (sizeOut > maxDecompressedTileBytes)
        sizeOut = maxDecompressedTileBytes;
    if (sizeOut < TILE_SIZE_4BPP)
    {
        Free(ptr);
        return;
    }
    tileCount = (u16)(sizeOut / TILE_SIZE_4BPP);
    for (t = 0; t < tileCount; t++)
    {
        u16 logical = (u16)(logicalFirstTileIndex + t);
        u16 dest = (u16)(gFieldMapLogicalToPhysicalTileId[logical] & 0x3FFu);
#ifdef PORTABLE
        if (!FieldPortableFieldSingleMappedTileDestOk(dest))
        {
            AGB_ASSERT(FALSE);
            Free(ptr);
            return;
        }
#endif
        {
            u16 c = LoadBgTiles(MAP_BG_FIELD_TILESET_GFX_BG, (u8 *)ptr + t * TILE_SIZE_4BPP, TILE_SIZE_4BPP, dest);

            FieldMapSyncWaitForBgTilesDmaRequestIfValid(c);
        }
    }
    Free(ptr);
}

static void CopyTilesetToVram(struct Tileset const *tileset, u16 numTiles, u16 offset)
{
    if (tileset)
    {
        if (!tileset->isCompressed)
        {
#ifdef PORTABLE
            {
                const u8 *src = (const u8 *)FireredPortableCompiledTilesetTilesForLoad(tileset);
                u32 tb = FireredPortableCompiledTilesetVramTileBytesForLoad(tileset, numTiles);
                u16 t;

                if (FieldMapFieldTileUploadUsesIdentityLayout())
                {
                    if (!FieldPortableFieldTilesetBgUploadContractOk(tb, offset, TRUE))
                    {
                        AGB_ASSERT(FALSE);
                        return;
                    }
                    LoadBgTiles(MAP_BG_FIELD_TILESET_GFX_BG, src, (u16)tb, offset);
                }
                else
                {
                    for (t = 0; t < numTiles; t++)
                    {
                        u16 logical = (u16)(offset + t);
                        u16 dest = (u16)(gFieldMapLogicalToPhysicalTileId[logical] & 0x3FFu);

                        if (!FieldPortableFieldSingleMappedTileDestOk(dest))
                        {
                            AGB_ASSERT(FALSE);
                            return;
                        }
                        LoadBgTiles(MAP_BG_FIELD_TILESET_GFX_BG, src + t * TILE_SIZE_4BPP, TILE_SIZE_4BPP, dest);
                    }
                }
            }
#else
            {
                if (FieldMapFieldTileUploadUsesIdentityLayout())
                    LoadBgTiles(MAP_BG_FIELD_TILESET_GFX_BG, tileset->tiles, numTiles * TILE_SIZE_4BPP, offset);
                else
                {
                    const u8 *src = (const u8 *)tileset->tiles;
                    u16 t;

                    for (t = 0; t < numTiles; t++)
                    {
                        u16 logical = (u16)(offset + t);
                        u16 dest = (u16)(gFieldMapLogicalToPhysicalTileId[logical] & 0x3FFu);

                        LoadBgTiles(MAP_BG_FIELD_TILESET_GFX_BG, src + t * TILE_SIZE_4BPP, TILE_SIZE_4BPP, dest);
                    }
                }
            }
#endif
        }
        else
#ifdef PORTABLE
        {
            u32 tb = FireredPortableCompiledTilesetVramTileBytesForLoad(tileset, numTiles);

            if (FieldMapFieldTileUploadUsesIdentityLayout())
            {
                if (!FieldPortableFieldTilesetBgUploadContractOk(tb, offset, FALSE))
                {
                    AGB_ASSERT(FALSE);
                    return;
                }
                DecompressAndCopyTileDataToVram2(MAP_BG_FIELD_TILESET_GFX_BG, FireredPortableCompiledTilesetTilesForLoad(tileset),
                    tb, offset, 0);
            }
            else
            {
                FieldMapCopyCompressedFieldTileSpanRemapped(FireredPortableCompiledTilesetTilesForLoad(tileset), tb, offset);
            }
        }
#else
        {
            if (FieldMapFieldTileUploadUsesIdentityLayout())
                DecompressAndCopyTileDataToVram2(MAP_BG_FIELD_TILESET_GFX_BG, tileset->tiles, numTiles * TILE_SIZE_4BPP, offset, 0);
            else
                FieldMapCopyCompressedFieldTileSpanRemapped(tileset->tiles, (u32)numTiles * TILE_SIZE_4BPP, offset);
        }
#endif
    }
}

static void CopyTilesetToVramUsingHeap(struct Tileset const *tileset, u16 numTiles, u16 offset)
{
    if (tileset)
    {
        if (!tileset->isCompressed)
        {
#ifdef PORTABLE
            {
                const u8 *src = (const u8 *)FireredPortableCompiledTilesetTilesForLoad(tileset);
                u32 tb = FireredPortableCompiledTilesetVramTileBytesForLoad(tileset, numTiles);
                u16 t;

                if (FieldMapFieldTileUploadUsesIdentityLayout())
                {
                    if (!FieldPortableFieldTilesetBgUploadContractOk(tb, offset, TRUE))
                    {
                        AGB_ASSERT(FALSE);
                        return;
                    }
                    LoadBgTiles(MAP_BG_FIELD_TILESET_GFX_BG, src, (u16)tb, offset);
                }
                else
                {
                    for (t = 0; t < numTiles; t++)
                    {
                        u16 logical = (u16)(offset + t);
                        u16 dest = (u16)(gFieldMapLogicalToPhysicalTileId[logical] & 0x3FFu);

                        if (!FieldPortableFieldSingleMappedTileDestOk(dest))
                        {
                            AGB_ASSERT(FALSE);
                            return;
                        }
                        LoadBgTiles(MAP_BG_FIELD_TILESET_GFX_BG, src + t * TILE_SIZE_4BPP, TILE_SIZE_4BPP, dest);
                    }
                }
            }
#else
            {
                if (FieldMapFieldTileUploadUsesIdentityLayout())
                    LoadBgTiles(MAP_BG_FIELD_TILESET_GFX_BG, tileset->tiles, numTiles * TILE_SIZE_4BPP, offset);
                else
                {
                    const u8 *src = (const u8 *)tileset->tiles;
                    u16 t;

                    for (t = 0; t < numTiles; t++)
                    {
                        u16 logical = (u16)(offset + t);
                        u16 dest = (u16)(gFieldMapLogicalToPhysicalTileId[logical] & 0x3FFu);

                        LoadBgTiles(MAP_BG_FIELD_TILESET_GFX_BG, src + t * TILE_SIZE_4BPP, TILE_SIZE_4BPP, dest);
                    }
                }
            }
#endif
        }
        else
#ifdef PORTABLE
        {
            u32 tb = FireredPortableCompiledTilesetVramTileBytesForLoad(tileset, numTiles);

            if (FieldMapFieldTileUploadUsesIdentityLayout())
            {
                if (!FieldPortableFieldTilesetBgUploadContractOk(tb, offset, FALSE))
                {
                    AGB_ASSERT(FALSE);
                    return;
                }
                DecompressAndLoadBgGfxUsingHeap2(MAP_BG_FIELD_TILESET_GFX_BG, FireredPortableCompiledTilesetTilesForLoad(tileset),
                    tb, offset, 0);
            }
            else
            {
                FieldMapCopyCompressedFieldTileSpanRemapped(FireredPortableCompiledTilesetTilesForLoad(tileset), tb, offset);
            }
        }
#else
        {
            if (FieldMapFieldTileUploadUsesIdentityLayout())
                DecompressAndLoadBgGfxUsingHeap2(MAP_BG_FIELD_TILESET_GFX_BG, tileset->tiles, numTiles * TILE_SIZE_4BPP, offset, 0);
            else
                FieldMapCopyCompressedFieldTileSpanRemapped(tileset->tiles, (u32)numTiles * TILE_SIZE_4BPP, offset);
        }
#endif
    }
}

static void ApplyGlobalTintToPaletteEntries(u16 offset, u16 size)
{
    switch (gGlobalFieldTintMode)
    {
    case QL_TINT_NONE:
        return;
    case QL_TINT_GRAYSCALE:
        TintPalette_GrayScale(&gPlttBufferUnfaded[offset], size);
        break;
    case QL_TINT_SEPIA:
        TintPalette_SepiaTone(&gPlttBufferUnfaded[offset], size);
        break;
    case QL_TINT_BACKUP_GRAYSCALE:
        QuestLog_BackUpPalette(offset, size);
        TintPalette_GrayScale(&gPlttBufferUnfaded[offset], size);
        break;
    default:
        return;
    }
    CpuCopy16(&gPlttBufferUnfaded[offset], &gPlttBufferFaded[offset], PLTT_SIZEOF(size));
}

void ApplyGlobalTintToPaletteSlot(u8 slot, u8 count)
{
    switch (gGlobalFieldTintMode)
    {
    case QL_TINT_NONE:
        return;
    case QL_TINT_GRAYSCALE:
        TintPalette_GrayScale(&gPlttBufferUnfaded[BG_PLTT_ID(slot)], count * 16);
        break;
    case QL_TINT_SEPIA:
        TintPalette_SepiaTone(&gPlttBufferUnfaded[BG_PLTT_ID(slot)], count * 16);
        break;
    case QL_TINT_BACKUP_GRAYSCALE:
        QuestLog_BackUpPalette(BG_PLTT_ID(slot), count * 16);
        TintPalette_GrayScale(&gPlttBufferUnfaded[BG_PLTT_ID(slot)], count * 16);
        break;
    default:
        return;
    }
    CpuFastCopy(&gPlttBufferUnfaded[BG_PLTT_ID(slot)], &gPlttBufferFaded[BG_PLTT_ID(slot)], count * PLTT_SIZE_4BPP);
}

static void LoadTilesetPalette(struct Tileset const *tileset, u16 destOffset, u16 size)
{
    u16 black = RGB_BLACK;

    if (tileset)
    {
        if (tileset->isSecondary == FALSE)
        {
#ifdef PORTABLE
            const u16 *palRow0 = FireredPortableCompiledTilesetPaletteRowPtr(tileset, 0u);
#else
            const u16 *palRow0 = tileset->palettes[0];
#endif
            LoadPalette(&black, destOffset, PLTT_SIZEOF(1));
            LoadPalette(palRow0 + 1, destOffset + 1, size - PLTT_SIZEOF(1));
            ApplyGlobalTintToPaletteEntries(destOffset + 1, (size - 2) >> 1);
        }
        else if (tileset->isSecondary == TRUE)
        {
#ifdef PORTABLE
            const u16 *palSec = FireredPortableCompiledTilesetPaletteRowPtr(tileset, (u32)FireredPortableFieldMapBgPalettePrimaryRows());
#else
            const u16 *palSec = tileset->palettes[MAP_BG_SECONDARY_TILESET_PALETTE_ROW_INDEX];
#endif
            LoadPalette(palSec, destOffset, size);
            ApplyGlobalTintToPaletteEntries(destOffset, size >> 1);
        }
        else
        {
            LoadCompressedPalette((const u32 *)tileset->palettes, destOffset, size);
            ApplyGlobalTintToPaletteEntries(destOffset, size >> 1);
        }
    }
}

void CopyPrimaryTilesetToVram(const struct MapLayout *mapLayout)
{
    CopyTilesetToVram(mapLayout->primaryTileset, MAP_BG_PRIMARY_TILESET_TILE_COUNT, MAP_BG_PRIMARY_TILESET_CHAR_BASE_TILE);
}

void CopySecondaryTilesetToVram(const struct MapLayout *mapLayout)
{
    CopyTilesetToVram(mapLayout->secondaryTileset, MAP_BG_SECONDARY_TILESET_TILE_COUNT, MAP_BG_SECONDARY_TILESET_CHAR_BASE_TILE);
}

void CopySecondaryTilesetToVramUsingHeap(const struct MapLayout *mapLayout)
{
    CopyTilesetToVramUsingHeap(mapLayout->secondaryTileset, MAP_BG_SECONDARY_TILESET_TILE_COUNT, MAP_BG_SECONDARY_TILESET_CHAR_BASE_TILE);
}

static void LoadPrimaryTilesetPalette(const struct MapLayout *mapLayout)
{
#ifdef PORTABLE
    u16 priRows = (u16)FireredPortableFieldMapBgPalettePrimaryRows();
    LoadTilesetPalette(mapLayout->primaryTileset, BG_PLTT_ID(0), (u16)((u32)priRows * PLTT_SIZE_4BPP));
#else
    LoadTilesetPalette(mapLayout->primaryTileset, BG_PLTT_ID(0), MAP_BG_PRIMARY_PALETTE_BYTES);
#endif
}

void LoadSecondaryTilesetPalette(const struct MapLayout *mapLayout)
{
#ifdef PORTABLE
    u16 priRows = (u16)FireredPortableFieldMapBgPalettePrimaryRows();
    u16 secRows = (u16)FireredPortableFieldMapBgPaletteSecondaryRows();
    LoadTilesetPalette(mapLayout->secondaryTileset, BG_PLTT_ID(priRows), (u16)((u32)secRows * PLTT_SIZE_4BPP));
#else
    LoadTilesetPalette(mapLayout->secondaryTileset, BG_PLTT_ID(MAP_BG_SECONDARY_PALETTE_START_SLOT), MAP_BG_SECONDARY_PALETTE_BYTES);
#endif
}

void CopyMapTilesetsToVram(struct MapLayout const *mapLayout)
{
    if (mapLayout)
    {
        CopyTilesetToVramUsingHeap(mapLayout->primaryTileset, MAP_BG_PRIMARY_TILESET_TILE_COUNT, MAP_BG_PRIMARY_TILESET_CHAR_BASE_TILE);
        CopyTilesetToVramUsingHeap(mapLayout->secondaryTileset, MAP_BG_SECONDARY_TILESET_TILE_COUNT, MAP_BG_SECONDARY_TILESET_CHAR_BASE_TILE);
    }
}

void LoadMapTilesetPalettes(struct MapLayout const *mapLayout)
{
    if (mapLayout)
    {
        LoadPrimaryTilesetPalette(mapLayout);
        LoadSecondaryTilesetPalette(mapLayout);
    }
}
