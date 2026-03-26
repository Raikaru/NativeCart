# Portable ROM: map header scripts directory (`MapHeader.mapScripts`)

## Purpose

Optional **ROM-backed** override for **`gMapHeader.mapScripts`** — the tagged byte stream consumed by **`MapHeaderGetScriptTable`** / **`RunOnLoadMapScript`** / **`RunOnTransitionMapScript`** / frame table hooks in **`script.c`**. Embedded script pointers in the stream still go through **`ResolveMapScriptPointer`** on **PORTABLE** (same as compiled data).

On **PORTABLE**, **`LoadCurrentMapData`** / **`LoadSaveblockMapHeaderWithLayoutSync`** call **`ResolveRomPointer(gMapHeader.mapScripts)`** immediately after **`ResolveRomPointer(gMapHeader.events)`** ( **`src/overworld.c`** and **`src_transformed/overworld.c`** ) so a header that stores **`mapScripts`** as a raw **`u32`/token word** still yields a usable base pointer before the optional directory override; real host pointers (typical **`map_data_portable`** arrays) pass through unchanged.

## Indexing

Dense grid keyed like **`FIRERED_ROM_MAP_HEADER_SCALARS_PACK_OFF`** / **`FIRERED_ROM_MAP_CONNECTIONS_PACK_OFF`**:

- **`MAP_GROUPS_COUNT × 256`** rows (**`include/constants/map_groups.h`**, **`FIRERED_ROM_MAP_SCRIPTS_MAPNUM_MAX`** = 256).
- Unused **`(mapGroup, mapNum)`** cells (**`mapNum >= gMapGroupSizes[group]`**) must be **8 zero bytes**.

## Wire row (8 bytes)

| Offset | Size | Content |
|--------|------|---------|
| **+0** | **4** | **`blob_off`** — little-endian **`u32`** **file offset** into the **mapped** ROM image (same convention as other **`FIRERED_ROM_*_OFF`** env vars). |
| **+4** | **4** | **`blob_len`** — little-endian **`u32`** length of the map-script table blob. |

### Semantics

- **`blob_off == 0` and `blob_len == 0`:** no ROM override for this map → keep **`mapScripts`** from the compiled **`struct MapHeader`** copied from **`gMapGroups`**.
- **`blob_off != 0` and `blob_len != 0`:** **`mapScripts = ENGINE_ROM + blob_off`** after validation.
- Any other combination → **pack rejected** (all-or-nothing).

## Blob shape (validation)

Must match the walk in **`MapHeaderGetScriptTable`**: repeating **5-byte** entries **`tag` + 4-byte pointer word** until **`tag == 0`**. The validator requires a **terminating zero tag** before **`blob_len`** is exhausted (padding after **`0` is allowed).

## Activation

1. **`FIRERED_ROM_MAP_SCRIPTS_DIRECTORY_PACK_OFF`** — hex file offset to the **start** of the grid.
2. Optional **`FireredRomU32TableProfileRow`** in **`firered_portable_rom_map_scripts_directory.c`**.

**`firered_portable_rom_map_scripts_directory_refresh_after_rom_load()`** runs during **`engine_backend_init`** (after map connections refresh). **`firered_portable_rom_map_scripts_directory_merge`** runs in **`LoadCurrentMapData` / `LoadSaveblockMapHeader`** after connections merge.

## Compiled fallback

If the pack is missing or invalid, **`mapScripts`** stays the compiled pointer from **`map_data_portable`**.

## Half-migration warning

ROM **header scalars / connections / events** can disagree with ROM **mapScripts** for the same map; this seam does **not** enforce cross-field consistency beyond per-row bounds.

## Related

- **`cores/firered/portable/firered_portable_rom_map_scripts_directory.c`**
- **`include/map_scripts_access.h`**
- **`src_transformed/script.c`** (`MapHeaderGetScriptTable`)
- **`docs/portable_rom_map_generated_data_playbook.md`**
- **`tools/portable_generators/validate_map_scripts_directory_pack.py`**
