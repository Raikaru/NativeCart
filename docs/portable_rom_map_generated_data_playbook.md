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
3. **Inventory scale** — run **`python tools/inventory_map_generated_data.py`** (and optionally **`python tools/list_generated_data_candidates.py --grep map`**) so merges and editor opens are deliberate; prefer **`rg`** for text lookups and **`ast-grep`** when you need syntax-aware C/C++ searches.
4. **Pointer vs pointer-free** — map aggregates mix **`extern` forward declarations**, **`static const` arrays**, and (for some seams) **ROM `0x08……` words**. Classify your target slice before documenting **`FIRERED_ROM_*`** names; pointer tables use **`--gba-u32-pointers`** in **`tools/rom_blob_inspect.py`** (see below).

## 3. Suggested validation patterns (env + profile + blob)

1. **Environment variable** — follow existing naming: **`FIRERED_ROM_<AREA>_<TABLE>_OFF`** (hex offset into the **mapped ROM file**), documented in a focused **`docs/portable_rom_<topic>_table.md`** peer. Discover patterns: `rg "FIRERED_ROM_.*_OFF" docs`.
2. **Profile row** — when your build uses shared portable profiles, mirror how **`docs/rom_backed_runtime.md`** ties env vars to **profile** fields for other tables (same activation and fail-open story).
3. **Byte proof (read-only)** — **`python tools/rom_blob_inspect.py <rom.gba> -o <off> -n <bytes> --expect-len <bytes>`** plus, where applicable, **`--struct-stride`**, **`--gba-u32-pointers`**, or **`--u8-cols`**. Full flag reference: **`docs/rom_blob_inspection_playbook.md`**.
4. **Coherent blob checklist** — before any migration, walk **`docs/generated_data_rom_seam_playbook.md`** §7 (one index contract, companion tables, dimensions, pointer closure).
5. **Header scalars vs layout metatiles** — wire **`mapLayoutId`** may override compiled headers; effective **`MapLayout`** via **`FireredPortableEffectiveMapLayoutForLayoutId`** (optional **`FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF`**). See **`docs/portable_rom_map_header_scalars_pack.md`** and **`docs/portable_rom_map_layout_metatiles.md`**.

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
| **`docs/portable_rom_map_connections_pack.md`** | Dense **`(mapGroup, mapNum)`** grid for **`gMapHeader.connections`** (optional ROM pack). |
| **`tools/portable_generators/validate_map_connections_pack.py`** | Offline structural checks on that connections grid. |
| **`docs/portable_rom_map_scripts_directory.md`** | Dense grid: per-map **`mapScripts`** blob **file offset + length** into mapped ROM. |
| **`tools/portable_generators/validate_map_scripts_directory_pack.py`** | Offline structural checks on that directory + blob layout. |
| **`docs/portable_rom_map_events_directory.md`** | Dense grid: per-map **`MapEvents`** blob (objects / warps / coord / bg) into mapped ROM. |

## 6. Tooling

| Tool | Purpose |
|------|---------|
| **`tools/inventory_map_generated_data.py`** | Read-only: map-related **`cores/firered/generated/data/*.c`**, line counts, resolved include target, rough **`static const`** (and related) counts for seam sizing. |
| **`tools/list_generated_data_candidates.py`** | Sort all generated **`.c`/`.h`** by size under **`pokefirered_core/generated`**; use **`--grep map`** to focus. |
| **`tools/rom_blob_inspect.py`** | Validate ROM slices before documenting offsets. |
| **`tools/portable_generators/build_map_events_rom_pack.py`** | Optional **`FIRERED_ROM_MAP_EVENTS_DIRECTORY_PACK_OFF`** image from **`map.json`**. |
| **`tools/portable_generators/validate_map_events_directory_pack.py`** | Offline structural checks on that pack layout. |
| **`tools/portable_generators/validate_map_scripts_directory_pack.py`** | Offline structural checks for **`FIRERED_ROM_MAP_SCRIPTS_DIRECTORY_PACK_OFF`** grid + tagged blobs. |
| **`tools/portable_generators/build_map_connections_rom_pack.py`** | Optional **`FIRERED_ROM_MAP_CONNECTIONS_PACK_OFF`** image from **`data/maps/**` JSON. |
| **`tools/portable_generators/validate_map_connections_pack.py`** | Offline structural checks on that pack layout. |
| **`tools/portable_generators/build_map_layout_metatiles_rom_pack.py`** | Optional **`FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF`** image from **`data/layouts/layouts.json`** + **`border.bin`** / **`map.bin`**. |
| **`tools/portable_generators/build_map_layout_dimensions_rom_pack.py`** | Optional **`FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF`** dense table (**`u16`** × 4 per layout slot) from **`layouts.json`**. |
| **`tools/portable_generators/build_map_layout_tileset_indices_rom_pack.py`** | Optional **`FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF`** (**`u16`** primary/secondary index per slot; **`0xFFFF`** = compiled / NULL slot). |
| **`tools/portable_generators/build_map_layout_rom_companion_bundle.py`** | **One command:** build all three companion blobs, concatenate **`[metatile][dimensions][tileset indices]`**, write **`build/offline_map_layout_rom_companion_bundle.bin` + `.manifest.txt`**, full **`validate_map_layout_rom_packs.py`** (offline gates / CI). |
| **`tools/portable_generators/layout_rom_companion_emit_embed_env.py`** | Read **`.manifest.txt`** + **`--bundle-base-offset P`** → **`FIRERED_ROM_*_OFF`** exports, **`rom_blob_inspect.py`** / **`validate_map_layout_rom_packs.py`** examples for the host ROM. |
| **`tools/portable_generators/layout_rom_companion_splice_bundle.py`** | Copy **`.gba`** / host image → output path; write bundle at **`--offset P`** when **`P == METATILE_PACK_BASE_OFFSET`**; optional **`--validate`**. |
| **`tools/portable_generators/layout_rom_companion_prepare_test_rom.py`** | One command: build bundle with **`--metatile-pack-base-offset P`**, splice into **`--output-rom`**, emit **`FIRERED_ROM_*_OFF`** (+ inspect/validate snippets); **`--validate`** validates the spliced ROM. |
| **`tools/portable_generators/layout_rom_companion_audit_rom_placement.py`** | Read-only: bounds + slice stats for **`P`**; optional **trailing-fill heuristic** for a candidate **`P`** (not proof of free space). |
| **`tools/portable_generators/append_validate_map_layout_meta_dim_combined.py`** | Metatile + dimensions only (older two-way cross-check; full stack → companion bundle). |
| **`tools/portable_generators/validate_map_layout_rom_packs.py`** | Structural checks for metatile, optional dimensions (**`--dimensions-off` alone** allowed), optional tileset-index directory; **`--layouts-json`** for slot count + NULL slots. |

## 7. Planning: next **deep** map/generated-data target (post map-header trio + wild pack)

Use this ranking when choosing an implementation pass **without** full **`map_data_portable.c`** replacement.

| Priority | Target | Rationale |
|----------|--------|-----------|
| **1 (recommended)** | **Layout / metatile / tileset-index ROM cohesion** — orchestrated **emitters** + validation from **`data/layouts/layouts.json`** and layout **`.bin`** files, covering **`FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF`**, optional **`FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF`**, optional **tileset-indices** / compiled-tileset asset rows as one workflow | **Highest gameplay payoff** for map hacks (geometry, tiles, metatiles). Runtime loaders and offline validators already exist (**`docs/portable_rom_map_layout_metatiles.md`**, **`validate_map_layout_rom_packs.py`**, tileset validators). **`layout_rom_companion_prepare_test_rom.py`** is the **one-shot** test-ROM path; granular scripts remain for ad hoc steps. Remaining **deep** work: CI or docs that exercise a **real** retail-sized ROM + full portable launch with env vars (optional). |
| **2 (fallback)** | **Oak speech / main-menu text** ROM slices (**`docs/rom_backed_runtime.md`** §8) | **High visibility** for dialogue hacks; **bounded** string family; **orthogonal** to `map_data_portable.c` bulk but still “generated-data shaped.” |
| **3 (defer)** | **Full `gMapLayouts[]` / monolithic `map_data_portable.c` teardown** | **Extreme** pointer + companion-table risk; only after layout packs and header/events/scripts/connections seams are routine in practice. |
| **Lower** | **`map_object_event_scripts_portable_data.c`** (~2.5k lines) as a *new* ROM table | Largely overlaps **`FIRERED_ROM_MAP_OBJECT_EVENT_SCRIPT_PTRS_OFF`** + resolver; incremental payoff **unless** the goal is eliminating the compiled **`gFireredPortableMapObjectEventScriptPtrs[]`** table entirely. |
| **Lower** | **Region map generated slices** | **§5p–§5s** / **`docs/portable_rom_region_map_*.md`** — many seams **already** implemented; smaller marginal win vs layout tooling. |
