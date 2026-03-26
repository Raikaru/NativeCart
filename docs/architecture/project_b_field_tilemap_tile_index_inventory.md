# Project B — Field BG tilemap / tile-index consumer inventory

**Purpose:** Call-site map for **Direction D** (strict caps) and **Direction B** (bounded logical→physical addressing — see **`project_b_field_tileset_redesign.md`** §5).  
**Scope:** Code paths that **read or write** overworld **map-related** tile **indices** in tilemaps or **upload** tiles into the **field** tileset BG (`MAP_BG_FIELD_TILESET_GFX_BG`, vanilla **BG2** chars), plus closely coupled **scratch** users.

**Not in scope:** Battle UI, naming screen, Pokédex, trade, etc. that reuse **BG2** or other BGs for **non-field** art (different `charBase` / template).

**Project C (metatile IDs):** Which **metatile row** (primary vs secondary **id** split) is documented in **`docs/architecture/project_c_metatile_id_map_format.md`** with **`MAP_GRID_METATILE_*`** helpers; **shop** / **Teachy TV** / **field_camera** / **fieldmap** attributes use those helpers. This inventory tracks **tile indices** inside metatile blobs and upload paths, not the **map grid metatile id** encoding.

---

## Overworld buffer ↔ GBA BG mapping (critical context)

From **`InitOverworldBgs`** (`overworld.c`):

| Buffer pointer        | `SetBgTilemapBuffer` BG id | Typical role on field        |
|-----------------------|----------------------------|------------------------------|
| `gBGTilemapBuffers2`  | **1**                      | Middle layer tilemap         |
| `gBGTilemapBuffers1`  | **2**                      | **Field map layer** tied to `MAP_BG_FIELD_TILESET_GFX_BG` chars |
| `gBGTilemapBuffers3`  | **3**                      | Bottom layer tilemap         |

**`field_camera.c`** writes **metatile** tile **halfwords** into these buffers (not into BG2’s buffer name directly — **BG2** uses **`gBGTilemapBuffers1`**).

---

## Call-site table

| Location | File(s) | Read/Write | What | Vanilla / contiguous tile-index assumptions | Direction B? |
|----------|---------|------------|------|---------------------------------------------|--------------|
| **Metatile → tilemaps** | `field_camera.c` | **W** `gBGTilemapBuffers1/2/3` | 8×`u16` per metatile from layout tilesets | Halfwords pass **`FieldMapTranslateMetatileTileHalfwordForFieldBg`** → **`FieldMapPhysicalTileIndexFromLogical`** → **`gFieldMapLogicalToPhysicalTileId`** (identity table today); flip/palette preserved | **Yes** — **first** translation choke point |
| **Redraw / camera** | `field_camera.c` (`DrawWholeMapView`, slices, `CurrentMapDrawMetatileAt`) | indirect **W** | Drives `DrawMetatileAt` | Same as above | **Yes** |
| **Tileset char upload** | `fieldmap.c` (`CopyTilesetToVram*`) | **W** VRAM via `LoadBgTiles` / decompress → **`MAP_BG_FIELD_TILESET_GFX_BG`** | **Combined logical** tile *i* in each tileset blob maps to **physical** slot **`gFieldMapLogicalToPhysicalTileId[offset + i]`** (`offset` = primary **0** / secondary **`MAP_BG_SECONDARY_TILESET_CHAR_BASE_TILE`**); identity → historical **one** bulk upload | **`MAP_BG_*`**, **`FieldMapFieldTileUploadUsesIdentityLayout()`**, PORTABLE DMA bounds | **Yes** — **Direction B** upload paired with **`FieldMapTranslateMetatileTileHalfwordForFieldBg`** |
| **Overworld BG init** | `overworld.c` | **W** buffer ptrs | Alloc + `SetBgTilemapBuffer` | Templates vs **`MAP_BG_FIELD_TILESET_GFX_BG`** verified **`FieldOverworldVerifyFieldTilesetBgAgreement`** (PORTABLE) | **No** |
| **Door animation** | `field_door.c` | **W** VRAM chars @ **`DOOR_TILE_START`** (= **`MAP_BG_FIELD_DOOR_TILE_RESERVED_FIRST`**); **W** tile halfwords via door metatile builder | **`BuildDoorTiles`**: `tileNum + i` in lower bits; halfwords go through **`FieldMapTranslateMetatileTileHalfwordForFieldBg`** | Scratch **8** tiles at **end** of field index space; **Direction B:** reserved logicals must map **to self** until upload + builder share remapping | **Partial** — coupled to **`gFieldMapLogicalToPhysicalTileId`** door contract |
| **Teachy TV** | `teachy_tv.c` | **R/W** heap buffer; **`LoadBgTiles(3, …)`** for display staging; **`GetBgTilemapBuffer(2)`** for RNG UI | Stitched map chars with **`MAP_BG_*_TILE_COUNT`** / **`FireredPortableCompiledTilesetVramTileBytesForLoad`** | Same **1024×32** combined buffer model as field | **Partial** — **BG3** upload for presentation; map data path uses **`MAP_BG_*`** |
| **DrawWholeMapView callers** | `field_specials.c`, `fldeff_cut.c`, … | **Call** | Forces full map redraw | Relies on **`field_camera`** | **Yes** (via camera) |
| **Portable tile / DMA guards** | `fieldmap.c`, `dma3_manager.c` | **Check** | Upload size, VRAM bounds | **`MAP_BG_*`**, **`ENGINE_VRAM`**-backed DMA | **No** |
| **Renderer sampling** | `engine_renderer_backend.c` (`engine_render_4bpp_tile`, text BG path) | **R** tilemap → **10-bit** index → VRAM | `charBase*0x4000 + index*32` | Matches GBA; field tilemaps must agree with uploaded chars | **Yes** — any **indirection** must land here (or in tilemap writer) |
| **Map preview screen** | `map_preview_screen.c` | **W** **BG0** tilemap from static previews | `CopyToBgTilemapBuffer(0, …)` | **Not** `gBGTilemapBuffers1/2/3`; **separate** UI | **No** (isolated) |
| **Help system map backup** | `help_system_util.c` | **R/W** **`BG_CHAR_ADDR(3)`** chars + buffered tilemap | `SaveMapTiles` / `RestoreMapTiles` / `CommitTilemap` | Uses **char block 3** for help UI, **not** field **`MAP_BG_FIELD_TILESET_GFX_BG`** path | **No** (different char base) |
| **Region map / switch** | `region_map.c` | **`LoadBgTiles(2, …)`** | Dungeon / switch UI tiles | Non-overworld templates | **No** |

---

## Gaps / follow-ups

- **Weather / `field_weather.c` / `field_effect.c`:** no direct **`gBGTilemapBuffers*`** or **`LoadBgTiles(MAP_BG_*)`** hits in a quick audit; effects mostly **scroll / palette / OBJ**. Re-audit if a specific effect patches **BG** tilemaps.
- **Shop:** no field **`gBGTilemapBuffers`** writes found; shop uses its **own** BG templates.
- **Validators:** see **`tools/portable_generators/validate_compiled_tileset_asset_profile_directory.py`** ( **`tile_uncomp_bytes`** cap = **`MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES`** ), **`validate_compiled_tileset_tiles_directory.py`**, **`validate_map_layout_metatile_graphics_directory.py`** — caps documented in script headers and aligned with **`include/fieldmap.h`**. **Direction B table:** **`tools/validate_field_map_logical_to_physical_inc.py`** on **`src/data/field_map_logical_to_physical_identity.inc`** (door tail, collision **warnings** or **`--strict-injective`**, optional **`--hint-primary-secondary`**).

---

## Contract vs redesign

- **Current contract (Direction D):** **`MAP_BG_*`** in **`fieldmap.h`**, upload + DMA guards, **`STATIC_ASSERT`s**, and **offline** caps on ROM profile / tile blobs.
- **Future (Direction B):** Any **indirection** must stay consistent across **`field_camera`** (writers), **`fieldmap`** (upload layout), and **`engine_renderer_backend.c`** (readers), and likely **ROM validators**.

See also: `docs/architecture/project_b_field_tileset_redesign.md`.
