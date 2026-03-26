# mapjson

Generates **asm** / **C** includes for **map headers**, **layout tables**, and **layout `.incbin`** references from `map.json` and `layouts.json`.

**Project C note:** Map **block** data (`border_filepath`, `blockdata_filepath`) is emitted as **raw `.incbin`** — this tool does **not** parse or validate cell payloads. Default shipped bins are **`u16`** PRET **wire V1** (`MAP_GRID_BLOCK_WORD_LAYOUT_VANILLA`): **10-bit** metatile, **2-bit** collision, **4-bit** elevation. Runtime uses **Phase 2 RAM** packing (see `map_grid_block_word.h`). After regenerating layouts, run **`python tools/validate_map_layout_block_bin.py --layouts-json data/layouts/layouts.json --repo-root .`** (or **`make check-map-layout-block-bins`**).

**Phase 3 (optional `u32` layout bins):** constants **`MAP_GRID_BLOCK_WORD_PHASE3_U32_*`** in `include/constants/map_grid_block_word.h`. Validate **`u32`** cells with **`python tools/validate_map_layout_block_bin.py --cell-width 4 …`** — do **not** pass **`--cell-width 4`** against vanilla **`u16`** `layouts.json` (cell counts / strides differ). ROM packs: `tools/portable_generators/validate_map_layout_rom_packs.py --validate-block-words` remains **wire V1 `u16`**. See `docs/architecture/project_c_map_block_word_and_tooling_boundary.md`.
