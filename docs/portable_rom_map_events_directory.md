# Portable ROM: map events directory (`MapHeader.events`)

## Purpose

Optional **ROM-backed** override for the full **`struct MapEvents`** aggregate attached to **`MapHeader`** — **object templates**, **warps**, **coord events**, and **bg events** (the same surface as vanilla **`map.json`** `object_events` / `warp_events` / `coord_events` / `bg_events`).

When active, **`firered_portable_rom_map_events_directory_merge`** replaces **`gMapHeader.events`** after the usual **`gMapGroups`** copy. On **PORTABLE**, **`LoadCurrentMapData`** / **`LoadSaveblockMapHeaderWithLayoutSync`** call **`ResolveRomPointer(gMapHeader.events)`** first (same in **`src/overworld.c`** and **`src_transformed/overworld.c`**) so compiled headers that store **`events`** as a raw **`u32`/ROM-band word** still yield a usable pointer before optional directory override.

## Indexing

Dense grid aligned with **`FIRERED_ROM_MAP_SCRIPTS_DIRECTORY_PACK_OFF`** / **`FIRERED_ROM_MAP_CONNECTIONS_PACK_OFF`**:

- **`MAP_GROUPS_COUNT × 256`** rows (**`include/constants/map_groups.h`**, **`FIRERED_ROM_MAP_EVENTS_MAPNUM_MAX`** = 256).
- Unused **`(mapGroup, mapNum)`** cells (**`mapNum >= gMapGroupSizes[group]`**) must be **8 zero bytes**.

## Wire row (8 bytes)

| Offset | Size | Content |
|--------|------|---------|
| **+0** | **4** | **`blob_off`** — little-endian **`u32`** **file offset** into the **mapped** ROM image. |
| **+4** | **4** | **`blob_len`** — little-endian **`u32`** length of the per-map events blob. |

### Semantics

- **`blob_off == 0` and `blob_len == 0`:** no ROM override → keep **`events`** from compiled **`map_data_portable`**.
- **`blob_off != 0` and `blob_len != 0`:** **`events`** points at static **`MapEvents`** built from the blob.
- Any other combination → **pack rejected** (all-or-nothing for the whole grid).

## Per-map blob header (16 bytes)

| Offset | Size | Content |
|--------|------|---------|
| **+0** | **4** | **Magic** **`0x5645504D`** (**`'MPEV'`** LE). |
| **+4** | **4** | **Version** — must be **`1`**. |
| **+8** | **4** | **Counts** — packed **`u8`**: low byte **object** count, then **warp**, **coord**, **bg** (maxima: **64** each; objects capped at **`OBJECT_EVENT_TEMPLATES_COUNT`** = **64**). |
| **+12** | **4** | **Reserved** — must be **0**. |

**Total payload length** must equal:

`16 + 24*obj + 8*warp + 16*coord + 12*bg`

## Row wire formats (after header)

### Object template (**24** bytes)

Raw **`struct ObjectEventTemplate`** under **`PORTABLE`** (including **`u32 script`**). **`_Static_assert`**’d in **`firered_portable_rom_map_events_directory.c`**.

- **`OBJ_KIND_CLONE`**: **`script`** must be **0**; clone target map must exist in **`gMapGroups`**.
- **`OBJ_KIND_NORMAL`**: non-zero **`script`** words must **`firered_portable_resolve_script_ptr`** to non-**NULL** (tokens, **`0x08……`** ROM addresses, etc.).

### Warp (**8** bytes)

Raw **`struct WarpEvent`** (little-endian fields). **`mapGroup` / `mapNum`** must refer to a non-**NULL** **`gMapGroups`** entry.

### Coord event (**16** bytes)

| Offset | Type | Notes |
|--------|------|--------|
| **+0** | **`u16`** **`x`** LE | |
| **+2** | **`u16`** **`y`** LE | |
| **+4** | **`u8`** **`elevation`** | |
| **+5** | **`u8`** **pad** | ignored |
| **+6** | **`u16`** **`trigger`** LE | |
| **+8** | **`u16`** **`index`** LE | |
| **+10** | **`u16`** **pad** | must be **0** (validated) |
| **+12** | **`u32`** **`script`** LE | **`firered_portable_resolve_script_ptr`**; **0** → **NULL** |

This matches a **16-byte GBA-aligned** record; host **`struct CoordEvent`** is larger because of pointers — the runtime **decodes** into **`struct CoordEvent`**.

### Bg event (**12** bytes)

| Offset | Type | Notes |
|--------|------|--------|
| **+0** | **`u16`** **`x`** LE | |
| **+2** | **`u16`** **`y`** LE | |
| **+4** | **`u8`** **`elevation`** | |
| **+5** | **`u8`** **`kind`** | |
| **+6** | **`u16`** **pad** | must be **0** |
| **+8** | **`u32`** **`payload`** LE | If **`kind`** is a **hidden-item** class (**`5`**, **`6`**, **`BG_EVENT_HIDDEN_ITEM`**) or **`BG_EVENT_SECRET_BASE`**: raw **`hiddenItem`** / secret-base id word. Else: script word through **`firered_portable_resolve_script_ptr`**. |

## Activation

1. **`FIRERED_ROM_MAP_EVENTS_DIRECTORY_PACK_OFF`** — hex file offset to the **start** of the directory grid **in the mapped ROM image** (same address space as **`ENGINE_ROM_ADDR`**).
2. Optional **`FireredRomU32TableProfileRow`** in **`firered_portable_rom_map_events_directory.c`**.

**`firered_portable_rom_map_events_directory_refresh_after_rom_load()`** runs during **`engine_backend_init`** (after map scripts directory refresh). **`firered_portable_rom_map_events_directory_merge`** runs in **`LoadCurrentMapData` / `LoadSaveblockMapHeader`** after scripts directory merge.

## Runtime validation contract (PORTABLE)

**All-or-nothing at refresh:** **`validate_pack()`** in **`firered_portable_rom_map_events_directory.c`** walks the **entire** **`MAP_GROUPS_COUNT × 256`** grid. It requires:

- Unused cells (**`mapNum >= gMapGroupSizes[group]`**) to be **8 zero bytes**.
- Used cells to have **`gMapGroups[group][mapNum] != NULL`**.
- Each **`0/0`** row: no blob.
- Each non-**`0/0`** row: **`validate_blob`** (magic, version, length equation, per-row semantic checks: warps vs **`gMapGroups`**, object scripts via **`firered_portable_resolve_script_ptr`**, coord/bg pads and script payloads).

If **any** check fails, **`firered_portable_rom_map_events_directory_refresh_after_rom_load`** leaves the seam **inactive** (`s_map_events_dir_rom_active == 0`); **`merge`** is then a no-op and **`gMapHeader.events`** stays on the **compiled** **`map_data_portable`** pointer (after **`ResolveRomPointer`**).

**Per-map merge:** **`merge`** does not re-validate; it assumes the pack still matches what refresh accepted. Decoding uses one static **`struct MapEvents`** plus fixed arrays (**`OBJECT_EVENT_TEMPLATES_COUNT`** objects, **64** warps/coords/bg max) — only the **current** map’s events live there after each load.

## Testing

1. **Offline:** `python tools/portable_generators/build_map_events_rom_pack.py <repo> out.bin --mode sparse` then `python tools/portable_generators/validate_map_events_directory_pack.py out.bin` (optional **`--map-groups`** for unused-row checks). A **sparse** pack validates and leaves per-map behavior unchanged (all **`0/0`** rows); if **`FIRERED_ROM_MAP_EVENTS_DIRECTORY_PACK_OFF`** points at it, refresh still accepts the pack and **`merge`** remains a no-op for every map.
2. **End-to-end:** Embed a **fill-all** or custom pack in the loaded ROM, set **`FIRERED_ROM_MAP_EVENTS_DIRECTORY_PACK_OFF`** to the grid’s offset, rebuild SDL debug, enter maps whose rows are non-**`0/0`** and confirm objects/warps/coord/bg behavior matches the blob.

## Compiled fallback

If the pack is missing or invalid, **`events`** stays the compiled pointer from **`map_data_portable`**.

## Emitter + validation tooling

| Script | Purpose |
|--------|---------|
| **`tools/portable_generators/build_map_events_rom_pack.py`** | Writes **`[grid][pool]`** image: **`--mode sparse`** (88064-byte zero grid) or **`--mode fill-all`** (vanilla blobs from **`data/maps/**/map.json`** + token indices from **`map_object_event_scripts_portable_data.c`** / **`event_scripts_portable_data.c`**). Needs **`gcc`** on **`PATH`** (macro expansion). |
| **`tools/portable_generators/validate_map_events_directory_pack.py`** | Structural proof on a pack file (magic, lengths, unused-slot zeros, script-word bands). Optional **`--map-groups`** for unused-row checks. |
| **`tools/portable_generators/load_c_macros_via_gcc.py`** | Shared **`gcc -E -dM`** helper for simple **`#define`** integers. |
| **`tools/portable_generators/eval_c_macro_int.py`** | Batches unevaluated macros (**`VAR_TEMP_*`**, etc.) via **`gcc -E -P`**. |

**Coord/bg script tokens:** **`generate_portable_event_script_data.py`** now appends every **`map.json`** coord/bg script label to **`gFireredPortableEventScriptPtrs[]`** (when that label exists in the parsed **`event_scripts.s`** tree) so **`0x81000000 + i`** encodings stay consistent for this pack and for **`firered_portable_resolve_script_ptr`**.

**Layout:** default output is **grid at file offset 0**, **pool** concatenated after **`MAP_GROUPS_COUNT × 256 × 8`** bytes; directory rows use **absolute file offsets** into the same image (same convention as **`docs/portable_rom_map_scripts_directory.md`**).

## Half-migration warning

ROM **events** can disagree with ROM **mapScripts**, **connections**, or **layout** for the same map; this seam does **not** enforce cross-field consistency beyond per-row bounds and resolver checks.

## Related

- **`cores/firered/portable/firered_portable_rom_map_events_directory.c`**
- **`include/map_events_access.h`**
- **`include/global.fieldmap.h`** (**`MapEvents`**, **`ObjectEventTemplate`**, **`WarpEvent`**, **`CoordEvent`**, **`BgEvent`**)
- **`docs/portable_rom_map_generated_data_playbook.md`**
