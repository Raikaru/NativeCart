# ROM-backed map layout metatile blobs (portable)

First **pointer-free** slice of `map_data_portable`: per-layout **`u16` border + map grids** only.  
`MapLayout` struct, `gMapGroups`, warps, connections, events, and tilesets are **unchanged**.

## Runtime API

- **`map_layout_metatiles_access.h`** — `MAP_LAYOUT_METATILE_MAP_PTR(ml)`, `MAP_LAYOUT_METATILE_BORDER_PTR(ml)`, and `firered_portable_rom_map_layout_metatiles_refresh_after_rom_load()` (PORTABLE).
- **`firered_portable_rom_map_layout_metatiles.c`** — implementation; **`firered_portable_rom_map_layout_metatiles.h`** is a thin include of the access header.

Consumers use the macros (`fieldmap.c`, `teachy_tv.c`); non-PORTABLE expands to `(ml)->map` / `(ml)->border`.

## Directory layout (ROM file offsets)

After `engine_memory_init`, the mapped ROM image starts at **`ENGINE_ROM_ADDR`**.  
**Base** of the directory: env / profile (see below).

For **`i = 0 .. gMapLayoutsSlotCount - 1`**, row **`i`** is **16 bytes**, little-endian **`u32`**:

| Offset in row | Field            | Meaning                                      |
|---------------|------------------|----------------------------------------------|
| 0             | `border_off`     | Absolute file offset of border `u16[]`       |
| 4             | `map_off`        | Absolute file offset of map `u16[]`          |
| 8             | `border_words`   | Word count = `borderWidth * borderHeight`    |
| 12            | `map_words`      | Word count = `width * height`                |

- **`gMapLayouts[i] == NULL`**: all four words **must be 0** (reserved / unused slot).
- **`gMapLayouts[i] != NULL`**: `border_words` / `map_words` **must** match the compiled `MapLayout` dimensions (same counts `generate_portable_map_data.py` / `read_u16_bin` use for `border.bin` / `map.bin`).  
  **All-or-nothing:** if any row fails validation, the module **clears** ROM-backed state and **falls back** to compiled `MapLayout.border` / `.map` pointers.

Offsets must be **2-byte aligned**; each blob must lie fully inside **`engine_memory_get_loaded_rom_size()`**.

## Env / profile

- **Env:** `FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF` — hex file offset to the first directory row (row 0 = same index as `gMapLayouts[0]`).
- **Profile:** `firered_portable_rom_u32_table_resolve_base_off` with placeholder profile id **`__firered_builtin_map_layout_metatiles_profile_none__`** (same pattern as other ROM table modules).

See also:

- `docs/portable_rom_map_generated_data_playbook.md`
- `docs/generated_data_rom_seam_playbook.md`
- `tools/inventory_map_generated_data.py`, `tools/rom_blob_inspect.py`

## Activation

`firered_portable_rom_map_layout_metatiles_refresh_after_rom_load()` runs from **`engine_backend_init`** after the ROM is mapped.  
Hot path uses a **sorted `MapLayout*` → slot index** table and **`bsearch`** (not a per-access scan of `gMapLayouts`).

## Hacks that benefit

ROMs or patches that **relayout metatile grids** without rebuilding portable `map_data_portable.c` can supply a directory + blobs at known offsets, **as long as** dimensions match the existing `MapLayout` table (slot order = `layouts.json` / generator order).

## Next migration steps (out of scope here)

- Map headers / `gMapGroups` / connections / `MapEvents` / object events  
- Tileset INCBIN / pointerful map data  
- Broader `map_data_portable.c` teardown after more ROM seams exist
