# ROM-backed region map section layout (`sMapSectionTopLeftCorners`, `sMapSectionDimensions`, PORTABLE)

## Purpose

Player cursor placement on the region map uses **`sMapSectionTopLeftCorners[mapsec][2]`** and **`sMapSectionDimensions[mapsec][2]`** (see **`src/region_map.c`**, **`GetPlayerPositionOnRegionMap`**). Together with the **`sRegionMapSections_*`** tensors, this defines coherent mapsec → tile behavior for fly / Pokédex maps.

## Layout contract (single pack)

**One** contiguous blob at **`FIRERED_ROM_REGION_MAP_SECTION_LAYOUT_PACK_OFF`**:

1. **`sMapSectionTopLeftCorners`** wire — **`MAPSEC_COUNT`** rows × **2** little-endian **`u16`** (x, y in region-map tile space).
2. **`sMapSectionDimensions`** wire — **`MAPSEC_COUNT`** rows × **2** little-endian **`u16`** (width, height in section tiles).

**Total:** **`MAPSEC_COUNT × 8`** bytes (e.g. **1616** when **`MAPSEC_COUNT`** is **202**).

## Validation (fail-safe)

- **Corners:** each **x** **&lt;** **`REGION_MAP_SECTION_GRID_WIDTH`**, **y** **&lt;** **`REGION_MAP_SECTION_GRID_HEIGHT`** (see **`include/constants/region_map_layout_dims.h`**).
- **Dimensions:** each dimension **≤ 32** (generous cap; **`MAPSEC_SPECIAL_AREA`** uses **`{0,0}`** in vanilla).

If validation fails, **both** active pointers stay **NULL** and **`region_map.c`** uses compiled **`sMapSectionTopLeftCorners_Compiled` / `sMapSectionDimensions_Compiled`**.

## Runtime wiring

- **Env / profile:** **`FIRERED_ROM_REGION_MAP_SECTION_LAYOUT_PACK_OFF`** (+ placeholder **`__firered_builtin_region_map_section_layout_profile_none__`**).
- **Refresh:** **`firered_portable_rom_region_map_section_layout_refresh_after_rom_load()`** from **`engine_runtime_backend.c`** (after section grids).
- **Access:** **`gMapSectionTopLeftCornersActive` / `gMapSectionDimensionsActive`** in **`region_map_section_layout_rom.h`**; **`#define`s** in **`region_map.c`**.

## Independence vs section grids

The **layout** pack and **`FIRERED_ROM_REGION_MAP_SECTION_GRIDS_PACK_OFF`** refresh **independently**. For hacked ROMs, prefer supplying **both** when geography changes.

## Source of compiled fallback

Portable builds use **`pokefirered_core/generated/src/data/region_map/region_map_entries.h`** (`*_Compiled` symbols). Regenerate via **`tools/portable_generators/generate_portable_region_map_overrides.py`** when JSON changes.
