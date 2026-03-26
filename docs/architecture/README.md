# Architecture notes (map / field visuals)

High-level planning and charters for overworld **map visuals** and related portable ROM seams.

**Program order (current):** **Project C Phase 2** **landed** (RAM **`u16`** widening + wire V1 on-disk). **Phase 3 tooling** **landed** — documented **`u32`** on-disk layout constants, **`validate_map_layout_block_bin.py --cell-width 4`**, **`check_project_c_phase3_offline.py`**, **`mapjson`** README — runtime / save / ROM still **Phase 2**. Docs: **`project_c_map_block_word_and_tooling_boundary.md`**, **`project_c_metatile_id_map_format.md`**. Offline: **`make check-offline-gates`** / **`python tools/run_offline_build_gates.py`** when **`make compare_*`** unavailable. **Direction D** / **bounded B** **closed**. **Next:** Phase 3 **runtime** **`u32`** grid or Phase 4 scope. **Lane A** / **Direction A** unchanged.

| Doc | Role |
|-----|------|
| [`map_visual_three_frontiers.md`](map_visual_three_frontiers.md) | Lanes A/B/C ranking (palette, VRAM/char layout, metatile / map format). |
| [`project_b_field_tileset_redesign.md`](project_b_field_tileset_redesign.md) | Larger **Project B** field tileset / char-plane program. |
| [`project_b_field_tilemap_tile_index_inventory.md`](project_b_field_tilemap_tile_index_inventory.md) | **Tile indices** in tilemaps and upload (Project B). |
| [`project_c_metatile_id_map_format.md`](project_c_metatile_id_map_format.md) | **Project C** — metatile **id** interpretation inside the map grid (`MAP_GRID_METATILE_*`, encoded space). |
| [`project_c_map_block_word_and_tooling_boundary.md`](project_c_map_block_word_and_tooling_boundary.md) | Project C **block word** boundary — layout id constants, `map_grid_block_word.h` choke point, save `mapView` hook, `mapjson` + validators. |

**Portable ROM (map layouts):** [`../portable_rom_map_layout_metatiles.md`](../portable_rom_map_layout_metatiles.md).

**Portable runtime / AGB bridge:** [`../rom_compat_architecture.md`](../rom_compat_architecture.md) — layered ROM/runtime model plus portable AGB-bridge guidance (renderer/input/timing/DMA centralization; use as a north star for bounded fixes, not as a rewrite mandate).

**CFRU / modern hack native port planning:** [`../cfru_integration_playbook.md`](../cfru_integration_playbook.md) — blocker inventory, compatibility-layer sketch, relation to ROM-backed tables and battle core. **Upstream links / build model:** [`../cfru_upstream_reference.md`](../cfru_upstream_reference.md).

**Vanilla / pret reference (not map-specific):** [`../upstream_reference_lane.md`](../upstream_reference_lane.md) — policy, **lane state** (bounded pret-shaped source queue **closed**), **metrics policy**, **pinned pret baseline (§0)** (add `pret` + `git fetch` locally to use it), drift categories, **observed** `compare_*`/ROM notes (**diagnostic**, not an assumed **`main`** gate).

**Tools:** `tools/mapjson/README.md`, `tools/docs/README_validators.md`.
