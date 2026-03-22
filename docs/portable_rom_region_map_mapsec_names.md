# ROM-backed region map mapsec display names (`GetMapName`, PORTABLE)

## Purpose

The region map UI shows a **map section name** string for the current cursor / fly target.
**`GetMapName`** indexes **`sMapNames[mapsec - KANTO_MAPSEC_START]`** for **`mapsec`** in
**`KANTO_MAPSEC_START … MAPSEC_NONE - 1`**.

On **PORTABLE**, **`sMapNames`** is **`gRegionMapMapsecNamesResolved[]`**: each slot is either
a pointer into the **mapped ROM** (GBA-encoded, EOS-terminated) or the compiled fallback
**`gRegionMapMapsecNames_Compiled[i]`** from **`region_map_entries.h`**.

## Entry count

**`REGION_MAP_MAPSEC_NAME_ENTRY_COUNT`** = **`(MAPSEC_NONE - KANTO_MAPSEC_START)`**
(see **`include/constants/region_map_mapsec_names.h`**). This matches the **`GetMapName`**
index bound; it is **not** the full **`MAPSEC_COUNT`** (out-of-range / special mapsecs use
other string paths).

## Resolution order (PORTABLE)

**`firered_portable_rom_region_map_mapsec_names_refresh_after_rom_load()`**:

1. **Pointer table (preferred, O(n))** — vanilla-shaped **`const u8 *const sMapNames[]`** in ROM:
   - **`FIRERED_ROM_REGION_MAP_MAPSEC_NAME_PTR_TABLE_OFF`** (hex file offset), or
   - **`FireredRomU32TableProfileRow`** rows in **`firered_portable_rom_region_map_mapsec_names.c`**
     (`s_region_map_mapsec_name_ptr_table_profile_rows`, same matching rules as other
     **`firered_portable_rom_u32_table`** users).
   - Table is **`REGION_MAP_MAPSEC_NAME_ENTRY_COUNT`** little-endian **`u32`** ARM ROM
     addresses; each must point into loaded ROM with a non-empty GBA string (**EOS** within
     **256** bytes). **All-or-nothing**: any bad row → skip this path.
2. Else **`firered_portable_rom_text_family_bind_all`** per entry:
   - **`try_profile_rom_off`** — tiny static **`s_mapsec_name_string_profile_rows`** table
     (**direct file offset** to one string, keyed by CRC and/or **`profile_id`** + entry
     index). Forks add rows; keep the table **small**.
   - Default **full-ROM scan** vs compiled bytes (**`scan_min_match_len = 6`**).
   - **`get_fallback`** → **`gRegionMapMapsecNames_Compiled[i]`**.

No per-index env key array (avoids hundreds of **`FIRERED_ROM_*`** vars).

On validation failure for an entry, the text-family step keeps the **compiled** pointer for
that row.

## Runtime wiring

- **Refresh:** after other region-map ROM slices in **`engine_runtime_backend.c`**.
- **Headers:** **`include/region_map_mapsec_names_rom.h`** (resolved table),
  **`include/constants/region_map_mapsec_names.h`** (count).

## Non-portable builds

Unchanged **`static const u8 *const sMapNames[]`** in **`region_map_entries.h`**; no ROM path.

## Follow-ups (product / architecture)

- Ship a **vanilla BPRE** profile row for **`s_region_map_mapsec_name_ptr_table_profile_rows`**
  once a stable **file offset** is recorded (tooling / `rom_blob_inspect`), so retail ROMs
  skip full-ROM scan without manual env.
- If scans stay ambiguous on a hack, prefer **pointer-table** env or **sparse string rows**
  over growing env surface.

**Celadon Dept. label:** **`GetMapName`** uses **`gRegionMapMapsecNamesResolved[MAPSEC_SPECIAL_AREA - KANTO_MAPSEC_START]`**
on **PORTABLE** when **`IsCeladonDeptStoreMapsec`** — the same GBA string as compiled
**`sMapsecName_CELADON_DEPT_`**, which is the **`MAPSEC_SPECIAL_AREA`** row in
**`gRegionMapMapsecNames_Compiled`**.
