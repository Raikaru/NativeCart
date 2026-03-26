# Project C — Metatile ID / map-format semantics (charter)

**Status:** **Phase 2 landed:** in-RAM / migrated `mapView` use **11-bit** metatile ids (**0..2047**) in the same **`u16`** cell (3-bit elevation); on-disk **`map.bin` / `border.bin`** stay **PRET wire V1** (10+2+4); **`MapGridBlockWordFromWireV1`** + **`NormalizeSavedMapViewBlockWords`**; **`gSaveBlock1Ptr->mapGridBlockWordSaveLayout`**. Compiled tilesets remain **640 + 384** rows unless extended via ROM/profile — ids **1024..2047** need larger tables.  
**Phase 3 (tooling):** C + Python constants + **`validate_map_layout_block_bin.py --cell-width 4`** for optional **`u32`** on-disk layout (**not** loaded by `fieldmap.c` yet).  
**Not in this program (yet):** runtime **`u32`** `VMap` / save / ROM loaders, **`mapjson`** emission of **`u32`** bins.

---

## Problem statement

Overworld **map grid** cells use a **`u16` block word**. **On disk** (PRET `map.bin`), the metatile field is **10-bit**; **in RAM** (Phase 2), it is **11-bit** after `MapGridBlockWordFromWireV1`. The **encoded id space** is split between two tilesets:

| Range (inclusive) | Tileset role | Row index into tables |
|-------------------|--------------|------------------------|
| `0 .. MAP_GRID_METATILE_PRIMARY_COUNT - 1` (today `0..639`) | Primary | Direct index |
| `MAP_GRID_METATILE_PRIMARY_COUNT .. MAP_GRID_METATILE_ID_SPACE_SIZE - 1` (today `640..2047`) | Secondary | `id - MAP_GRID_METATILE_PRIMARY_COUNT` |

**Graphics:** `field_camera.c` `DrawMetatileAt` chooses **`FireredMapLayoutMetatileGraphicsPtrForPrimary/Secondary`** (PORTABLE) or **`Tileset::metatiles`** and indexes with the same split.  
**Attributes / behaviors:** `GetAttributeByMetatileIdAndMapLayout` uses **`FireredMapLayoutMetatileAttributesPtrForPrimary/Secondary`** or compiled **`metatileAttributes`** with the same split.

This is **not** the same problem as Project B’s **tile index** inside each metatile’s 8×`u16` blob (`FieldMapTranslateMetatileTileHalfwordForFieldBg`, `MAP_BG_*` char layout). Project C is **which metatile row**; Project B is **which tile char** within the combined field BG once a row is chosen.

---

## Named policy (`include/fieldmap.h`)

Macros and `static inline` helpers document the contract and give a single edit anchor:

- **`MAP_GRID_METATILE_PRIMARY_COUNT`** — alias of **`NUM_METATILES_IN_PRIMARY`**
- **`MAP_GRID_METATILE_ID_SPACE_SIZE`** — **2048** (valid ids: `0 .. 2047`); **`NUM_METATILES_TOTAL`** remains **1024** for **compiled** table row counts
- **`MAP_GRID_METATILE_SECONDARY_COUNT`** — `ID_SPACE_SIZE - PRIMARY_COUNT` (**1408** secondary **ids** in the grid; vanilla compiled secondary still **384** rows)
- **`MapGridMetatileIdUsesPrimaryTileset(u32)`** / **`MapGridMetatileIdUsesSecondaryTileset(u32)`**
- **`MapGridMetatileIdIsInEncodedSpace(u32)`** — **`metatileId < MAP_GRID_METATILE_ID_SPACE_SIZE`** (matches **11-bit** **`MAPGRID_METATILE_ID_MASK`** span)
- **`MapGridMetatileNonPrimaryRowOffset(u32)`** / **`MapGridMetatileSecondaryRowIndex(u32)`** — secondary row index; use **`NonPrimaryRowOffset`** only after encoded-space normalization in draw/preview paths

### Out-of-encoded-space policy (landed)

| Path | Behavior |
|------|----------|
| **`DrawMetatileAt`** | If **`!MapGridMetatileIdIsInEncodedSpace`**, treat id as **0** (primary row 0), then primary vs non-primary branch + **`MapGridMetatileNonPrimaryRowOffset`**. |
| **`GetAttributeByMetatileIdAndMapLayout`** | If **`!MapGridMetatileIdIsInEncodedSpace`**, return **`0xFF`** for any attribute type (explicit early exit; same as former neither-primary-nor-secondary branch). |
| **Shop / Teachy TV** previews | Normalize invalid id to **0** before the same primary / non-primary split as the field draw path. |

**Vanilla note:** **`MapGridGetMetatileIdAt`** applies **`MAPGRID_METATILE_ID_MASK`**, so ids are always in encoded space on real map data; this policy mainly **documents** intent and guards **wider ids** if the grid format ever evolves.

---

## Call-site inventory (major consumers)

| Area | File(s) | Role |
|------|---------|------|
| **Graphics + camera** | `field_camera.c`, `src_transformed/field_camera.c` | `DrawMetatileAt` — **`MapGridMetatileIdIsInEncodedSpace`** → **0**, then **`MapGridMetatileNonPrimaryRowOffset`** |
| **Attributes** | `fieldmap.c`, `src_transformed/fieldmap.c` | `GetAttributeByMetatileIdAndMapLayout` — early **`0xFF`** if not in encoded space |
| **Grid read** | `fieldmap.c` | `MapGridGetMetatileIdAt` — mask only; no primary/secondary branch |
| **Shop buy-menu map** | `shop.c`, `src_transformed/shop.c` | **`BuyMenuDrawMapBg`** — normalize + same split as **`DrawMetatileAt`** |
| **Teachy TV** | `teachy_tv.c`, `src_transformed/teachy_tv.c` | Map tile build + palette entry — normalize + **`MapGridMetatileNonPrimaryRowOffset`** |
| **Portable ROM core** | `cores/firered/portable/firered_portable_rom_map_layout_metatiles.c` | **`metatile_attr_word_count`** uses **compiled** **`NUM_METATILES_*`** (640 / 384), not map-grid **1408** secondary span |
| **Portable ROM docs / validators** | `docs/portable_rom_map_layout_metatiles.md`, `tools/portable_generators/*.py` | ROM map/border blobs: **wire V1**; Python **`map_visual_policy_constants`** mirrors Phase 2 + wire |

---

## Relationship to portable ROM overlays

ROM-backed metatile **graphics/attributes** blobs may be **physically larger** than vanilla (profile-driven). **Map grid** ids may now reach **2047**; attribute/gfx lookup for **`id >= 1024`** requires secondary tables (or overlays) with enough rows — otherwise behavior is out-of-table (same class of risk as oversized profile tables pre–Phase 2).

---

## Phased roadmap (suggested)

1. **(Done)** Policy + helpers + shop + Teachy + portable row counts + **encoded-space** semantics (**draw / attribute / preview**).
1b. **(Done — drift gate)** Offline **`tools/check_project_c_block_word_offline.py`**: constants + Python + **`MAPGRID_*`** alias grep (runs in **`make check-validators`**, CI, **`verify_portable_default.*`**).
2. **(Done — Phase 2 in-RAM `u16`)** **11-bit** metatile field + **3-bit** elevation in RAM / migrated **`mapView`**; PRET **wire V1** on disk; **`mapGridBlockWordSaveLayout`**; **`NormalizeSavedMapViewBlockWords`**. Vanilla FRLG elevation data verified **<= 7** (fits 3 bits).
3. **(Done — Phase 3 tooling)** Documented **`u32`** on-disk cell layout + Python mirror + **`check_project_c_phase3_offline.py`** + **`validate_map_layout_block_bin.py --cell-width 4`**; **`mapjson`** README. **Not done:** runtime **`u32`** map grid / save migration / ROM loaders.
4. **Next (Phase 3 runtime / Phase 4):** **`u32`** **`VMap`**, save **`mapView`**, portable ROM map blobs, optional **mapjson** emission of **`u32`** bins — see **`project_c_map_block_word_and_tooling_boundary.md`**.

---

## Related docs

- `docs/architecture/README.md` — index of architecture notes (Project B/C cluster).
- `docs/architecture/project_c_map_block_word_and_tooling_boundary.md` — **u16** Phase 2 + wire V1, save / mask coupling, future **`u32`** track.
- `docs/architecture/map_visual_three_frontiers.md` — Lane C ranking (updated for Project C start).
- `docs/portable_rom_map_layout_metatiles.md` — ROM table sizes vs map id semantics.
- `docs/architecture/project_b_field_tilemap_tile_index_inventory.md` — tile **indices** (Project B), distinct from metatile **ids** (Project C).
- `tools/mapjson/README.md` — layout **incbin** emission (opaque block bytes).
