# Portable ROM: map-generated data seam (playbook)

Scoped notes for **planning and validating** work around **decomp-generated map data** in the ROM-backed portable architecture. This doc does **not** prescribe runtime wiring; it points to **read-only** workflows and **where the bytes actually live**.

## 1. How `map_data_portable` fits the build

| Layer | Path | Role |
|--------|------|------|
| **Core wrapper (compile unit)** | `cores/firered/generated/data/map_data_portable.c` | Almost always a **single `#include`** of the canonical source so the firered core build picks up the same generated file **without** duplicating it in-tree. |
| **Canonical generated body** | `pokefirered_core/generated/src/data/map_data_portable.c` | **Real** map layout, headers, scripts, and related `static const` rodata — **very large** (hundreds of thousands of lines). **Edits and regen happen here** (or in the generator inputs upstream), not in the thin wrapper. |
| **Related wrapper** | `cores/firered/generated/data/map_object_event_scripts_portable_data.c` | Same include-only pattern toward `pokefirered_core/generated/src/data/map_object_event_scripts_portable_data.c` (object-event script pointer tables / data used alongside maps). |

Portable runtime treats **`cores/firered/generated/data/*_portable*.c`** as part of the **compiled snapshot** path (see **`docs/rom_backed_runtime.md`** §1). A future **ROM-backed slice** for map-family data must respect **the same indexing and companion-table rules** as smaller seams (heal locations, wild headers, region map docs).

## 2. Prerequisites before slicing map data

1. **Confirm you are looking at the canonical file** — open or diff **`pokefirered_core/generated/src/data/map_data_portable.c`**, not only the wrapper under `cores/firered/generated/data/`.
2. **Establish a smaller proof first** — wild encounter headers, region map tables, or heal-location-style blobs validate **env offset + `rom_blob_inspect.py` + profile** without touching **`engine_runtime_backend.c`**, **`src_transformed/fieldmap.c`**, or **`portable_script_pointer_resolver.c`**. See **`docs/generated_data_rom_seam_playbook.md`** §3–§4 and §8.
3. **Inventory scale** — run **`python tools/inventory_map_generated_data.py`** (and optionally **`python tools/list_generated_data_candidates.py --grep map`**) so merges and editor opens are deliberate.
4. **Pointer vs pointer-free** — map aggregates mix **`extern` forward declarations**, **`static const` arrays**, and (for some seams) **ROM `0x08……` words**. Classify your target slice before documenting **`FIRERED_ROM_*`** names; pointer tables use **`--gba-u32-pointers`** in **`tools/rom_blob_inspect.py`** (see below).

## 3. Suggested validation patterns (env + profile + blob)

1. **Environment variable** — follow existing naming: **`FIRERED_ROM_<AREA>_<TABLE>_OFF`** (hex offset into the **mapped ROM file**), documented in a focused **`docs/portable_rom_<topic>_table.md`** peer. Discover patterns: `rg "FIRERED_ROM_.*_OFF" docs`.
2. **Profile row** — when your build uses shared portable profiles, mirror how **`docs/rom_backed_runtime.md`** ties env vars to **profile** fields for other tables (same activation and fail-open story).
3. **Byte proof (read-only)** — **`python tools/rom_blob_inspect.py <rom.gba> -o <off> -n <bytes> --expect-len <bytes>`** plus, where applicable, **`--struct-stride`**, **`--gba-u32-pointers`**, or **`--u8-cols`**. Full flag reference: **`docs/rom_blob_inspection_playbook.md`**.
4. **Coherent blob checklist** — before any migration, walk **`docs/generated_data_rom_seam_playbook.md`** §7 (one index contract, companion tables, dimensions, pointer closure).

## 4. Anti-patterns (map / layout context)

- **Treating the wrapper `.c` as the source of truth** — it is a build shim; **size and symbol counts** refer to the included path under **`pokefirered_core/generated`**.
- **First slice = full `map_data_portable.c`** — high merge cost and hard to validate; prefer a **bounded** sub-table or a **pointer-stable** header (e.g. wild mon headers) first.
- **Splitting companion arrays** — map indices, heal NPC ids, object events, and script pointer tables often **move together**; half-ROM / half-compiled hybrids break silently.
- **Routing experiments through merge-hot C** — avoid **`engine/core/engine_runtime_backend.c`**, **`src_transformed/fieldmap.c`**, **`cores/firered/generated/data/map_data_portable.c`**, and similar chokepoints **unless** the task explicitly requires it; prefer docs + inspect tools + small loaders in dedicated portable modules when wiring is unavoidable.

## 5. Related docs (read in this order)

| Doc | Use |
|-----|-----|
| **`docs/rom_blob_inspection_playbook.md`** | **`rom_blob_inspect.py`** flags, offset conventions, structural checks. |
| **`docs/generated_data_rom_seam_playbook.md`** | Generated-data layout, wild encounter seam shape, **`map_data_portable` scale warning**, coherent blob checklist, pointer-free pre-flight sheet. |
| **`docs/rom_backed_runtime.md`** | How **`FIRERED_ROM_*`** and profiles fit the portable stack; audit table mentioning **`cores/firered/generated/data/*_portable*.c`**. |
| **`docs/rom_compat_architecture.md`** | High-level load path and consolidation matrix. |
| **`docs/next_rom_structural_seam_notes.md`** | Broader “next seam” pointers (non-map families). |
| **`docs/portable_rom_region_map_*.md`** | Region map strings / sections / fly destinations — smaller “layout-ish” surfaces near map tooling. |
| **`docs/portable_rom_map_object_event_script_pointer_overlay.md`** | Object-event script pointer overlay (adjacent to map event data). |

## 6. Tooling

| Tool | Purpose |
|------|---------|
| **`tools/inventory_map_generated_data.py`** | Read-only: map-related **`cores/firered/generated/data/*.c`**, line counts, resolved include target, rough **`static const`** (and related) counts for seam sizing. |
| **`tools/list_generated_data_candidates.py`** | Sort all generated **`.c`/`.h`** by size under **`pokefirered_core/generated`**; use **`--grep map`** to focus. |
| **`tools/rom_blob_inspect.py`** | Validate ROM slices before documenting offsets. |
