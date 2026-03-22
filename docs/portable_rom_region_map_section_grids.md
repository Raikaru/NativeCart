# ROM-backed region map section grids (`sRegionMapSections_*`, PORTABLE)

## Purpose

The Pokédex / fly **region map** resolves `(map, layer, y, x)` → **`u8` map section id** via four parallel tensors:

- **`sRegionMapSections_Kanto`**
- **`sRegionMapSections_Sevii123`**
- **`sRegionMapSections_Sevii45`**
- **`sRegionMapSections_Sevii67`**

(see **`src/region_map.c`** → **`GetSelectedMapSection`**, layout in **`src/data/region_map/region_map_layout_*.h`**).

This slice is **pointer-free** and **map-adjacent**: hacks that redraw islands or Kanto geography can mirror a retail layout when the wire matches this build’s dimensions.

## Layout contract (single pack)

**One** contiguous blob at **`FIRERED_ROM_REGION_MAP_SECTION_GRIDS_PACK_OFF`**, in this order:

1. **Kanto** — **`REGION_MAP_SECTION_GRID_BYTE_SIZE`** bytes (`**2** layers × **15** rows × **22** cols × **`u8`** = **660**).
2. **Sevii 1–2–3** — **660** bytes.
3. **Sevii 4–5** — **660** bytes.
4. **Sevii 6–7** — **660** bytes.

**Total:** **`4 × 660 = 2640`** bytes.

Layer order matches **`region_map.c`**: index **`0`** = **`LAYER_MAP`**, **`1`** = **`LAYER_DUNGEON`**.

## Validation (fail-safe)

Every byte must satisfy **`(unsigned)b < (unsigned)MAPSEC_COUNT`**. On any failure, **all four** active pointers stay **NULL** and **`region_map.c`** uses compiled **`sRegionMapSections_*_Compiled`** (no mixing ROM Kanto with compiled Sevii).

## Runtime wiring

- **Env / profile:** **`FIRERED_ROM_REGION_MAP_SECTION_GRIDS_PACK_OFF`** (+ placeholder **`__firered_builtin_region_map_section_grids_profile_none__`**).
- **Refresh:** **`firered_portable_rom_region_map_section_grids_refresh_after_rom_load()`** from **`engine_runtime_backend.c`** (after heal locations).
- **Access:** **`gRegionMapSectionGrid*Active`** in **`region_map_section_grids_rom.h`**; **`#define sRegionMapSections_*`** in **`region_map.c`** after the layout includes.

## Companion layout ROM slice

**`sMapSectionTopLeftCorners`** / **`sMapSectionDimensions`** can be ROM-backed as a **paired pack** (**`FIRERED_ROM_REGION_MAP_SECTION_LAYOUT_PACK_OFF`**). See **`docs/portable_rom_region_map_section_layout.md`**. Other entry metadata (**`sMapNames`**, etc.) remains compiled.

## Non-portable builds

Unchanged symbol names in the layout headers; no ROM path.
