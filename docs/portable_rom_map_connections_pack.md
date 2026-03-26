# Portable ROM: map connections pack (`gMapHeader.connections`)

## Purpose

Optional **ROM-backed** replacement for **`struct MapConnections`** on the **current map** (`gMapHeader`), keyed by **`(mapGroup, mapNum)`** with the **same dense grid** as **`FIRERED_ROM_MAP_HEADER_SCALARS_PACK_OFF`**.

When active, **`firered_portable_rom_map_connections_merge()`** runs after each copy from **`gMapGroups[group][num]`** into **`gMapHeader`** (see **`LoadCurrentMapData` / `LoadSaveblockMapHeaderWithLayoutSync`**). On **PORTABLE**, **`gMapHeader.connections`** is passed through **`ResolveRomPointer()`** first (same treatment as **`events`** / **`mapScripts`**) so compiled header tokens decode to real pointers before merge; **Neighbor** maps still use compiled headers for layout/tile data; only **outgoing** connections from the **loaded** map are overridden.

## Activation (single switch)

1. **`FIRERED_ROM_MAP_CONNECTIONS_PACK_OFF`** — hex **file** offset into the mapped ROM image to the **start** of the dense grid.
2. **Profile rows** (optional): **`s_map_connections_profile_rows`** in **`firered_portable_rom_map_connections.c`** (`FireredRomU32TableProfileRow`), same resolution order as other ROM tables (**env first** via **`firered_portable_rom_u32_table_resolve_base_off`**).

If validation fails or the offset is unset, the module stays **inactive** and **`gMapHeader.connections`** remains the **compiled** pointer from **`map_data_portable`** (fail-open).

## Wire format

- **Grid size:** **`MAP_GROUPS_COUNT × 256`** rows ( **`include/constants/map_groups.h`** × **`FIRERED_ROM_MAP_CONNECTIONS_MAPNUM_MAX`** = 256).
- **Row stride:** **100** bytes.
- **Unused cells:** For **`mapNum >= gMapGroupSizes[mapGroup]`**, the entire **100** bytes must be **zero**.

### Per-row layout

| Offset | Size | Content |
|--------|------|---------|
| **+0** | **4** | Little-endian **`u32`**: **`count`** in **low 8 bits**; **bits 8–31 must be zero**. **`count`** ∈ **`0 .. 8`** (**`FIRERED_ROM_MAP_CONNECTIONS_MAX`**). |
| **+4** | **96** | **`8`** slots × **12** bytes each. Slots **`0 .. count-1`** are **active** **`struct MapConnection`** wire (host layout, little-endian). Slots **`count .. 7`** must be **all zero**. |

### `struct MapConnection` wire (12 bytes, must match portable `sizeof`)

| Offset | Type | Field |
|--------|------|--------|
| **+0** | **`u8`** | **`direction`** — **`CONNECTION_SOUTH`..`CONNECTION_EMERGE`** (**1..6**, see **`include/constants/global.h`**) |
| **+1..+3** | **pad** | **Zero** |
| **+4** | **`u32` LE** | **`offset`** (signed **`s32`** bit pattern) |
| **+8** | **`u8`** | **`mapGroup`** — must be **< `MAP_GROUPS_COUNT`** |
| **+9** | **`u8`** | **`mapNum`** — must be **< `gMapGroupSizes[mapGroup]`** |
| **+10..+11** | **pad** | **Zero** |

**Static assert:** **`firered_portable_rom_map_connections.c`** requires **`sizeof(struct MapConnection) == 12`**.

## Validation (all-or-nothing)

On refresh, the loader rejects the pack unless **all** of the following hold:

- The grid fits inside the loaded ROM file.
- Unused high **`mapNum`** rows (per group) are **fully zeroed**.
- For every **used** map cell, **`gMapGroups[group][num]`** is non-**NULL** (same expectation as map header scalars).
- **`count`** and reserved bits are valid; **active** slots have legal **direction** and **(mapGroup, mapNum)** with a **non-NULL** **`gMapGroups[targetGroup][targetNum]`**.
- **Inactive** slots (index **≥ count**) are **12** zero bytes each.

There is **no** mixed mode: either the **entire** grid validates and **`s_map_connections_rom_active`** is set, or the pack is ignored.

## Compiled fallback

When inactive, behavior matches the pre-migration path: **`gMapHeader.connections`** comes from the **compiled** **`struct MapHeader`** in **`map_data_portable`**.

## Tooling

- **`tools/portable_generators/build_map_connections_rom_pack.py`** — builds a **vanilla-shaped** pack from **`data/maps/**` JSON (same connection semantics as **`generate_portable_map_data.py`**). Connection targets use each map’s **`id`** field (**`MAP_*`**, e.g. **`MAP_ROUTE1`**) to resolve **`(mapGroup, mapNum)`**. **`MAP_GROUPS_COUNT`** in the script must match **`include/constants/map_groups.h`** for your tree.
- **`tools/portable_generators/validate_map_connections_pack.py`** — offline structural checks on the grid (and unused rows when **`--map-groups`** is set). Does **not** verify **`gMapGroups[group][num]`** / target map pointers (requires a running build).
- **`python tools/rom_blob_inspect.py`** — e.g. **`--struct-stride 100`** on the pack region for coarse sanity (mixed **`u8`/`u32`** per row; sub-slots may need separate slices).

## Related

- **`cores/firered/portable/firered_portable_rom_map_connections.c`**
- **`include/map_connections_access.h`**
- **`docs/portable_rom_map_generated_data_playbook.md`**
- **`docs/rom_backed_runtime.md`** (§5t)
