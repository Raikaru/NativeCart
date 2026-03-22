# ROM-backed `MapHeader` scalar pack (portable)

Pointer-free slice for **music, layout id, region map section, cave/weather/map type, flags, floor, battle scene** — the **`struct MapHeader` tail from `.music` through `.battleType`**. Pointers (`mapLayout`, `events`, `mapScripts`, `connections`) stay on the compiled `MapHeader` rodata.

## Indexing

Dense grid: **`MAP_GROUPS_COUNT` × 256** rows, **16 bytes** each (little-endian native layout of `FireredRomMapHeaderScalarsWire` in `firered_portable_rom_map_header_scalars.c`).

Row index: `(mapGroup * 256) + mapNum` (both treated as `u8` range in practice).

**Invalid cells** — any `(group, num)` with `num >= gMapGroupSizes[group]` — must be **all zero bytes**. Valid cells must pass sanity checks (`mapLayoutId` vs `gMapLayoutsSlotCount`, `regionMapSectionId` &lt; `MAPSEC_COUNT`, escape/run flags 0/1, `showMapName` ≤ 63).

`gMapGroupSizes[]` is emitted next to `gMapLayoutsSlotCount` in **`generate_portable_map_data.py`** / `map_data_portable.c`.

## Activation

- **Env / profile:** `FIRERED_ROM_MAP_HEADER_SCALARS_PACK_OFF` (hex file offset), with profile id **`__firered_builtin_map_header_scalars_profile_none__`** via `firered_portable_rom_u32_table_resolve_base_off`.
- **Refresh:** `firered_portable_rom_map_header_scalars_refresh_after_rom_load()` (engine init, after layout metatile directory refresh).
- **Runtime:** `firered_portable_rom_map_header_scalars_merge` patches **`gMapHeader`** after load; cross-map reads use **`FireredRomMapHeaderScalars*`** helpers (see `map_header_scalars_access.h`).

If validation fails, the module **clears** ROM state and **compiled scalars** are used everywhere.

## Related

- `docs/portable_rom_map_layout_metatiles.md` — layout border/map blobs (orthogonal).
- `docs/portable_rom_map_generated_data_playbook.md` — generated map data context.
