#include "global.h"
#include "tilesets.h"
#include "tileset_anims.h"

#ifdef PORTABLE
#include "tilesets_portable_assets_all.h"
#include "tilesets_portable_assets.h"
#define gTilesetTiles_GenericBuilding1 gTilesetTiles_GenericBuilding1_Portable
#define gTilesetPalettes_GenericBuilding1 gTilesetPalettes_GenericBuilding1_Portable
#else
#include "data/tilesets/graphics.h"
#include "data/tilesets/metatiles.h"
#endif

#include "data/tilesets/headers.h"
