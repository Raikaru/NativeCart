# Portable ROM: Pokémon battle sprite layout (pic coords + elevation)

Compiled data:

- **`gMonFrontPicCoords_Compiled[]`** — **`struct MonCoords`** per species (`src/data/pokemon_graphics/front_pic_coordinates.h`).
- **`gMonBackPicCoords_Compiled[]`** — same for back sprites (`back_pic_coordinates.h`).
- **`gEnemyMonElevation_Compiled[NUM_SPECIES]`** — per-species vertical offset for floating/flying enemies (`enemy_mon_elevation.h`).

**Front/back coord** tables are sized through **`SPECIES_UNOWN_QMARK`** (**`SPECIES_UNOWN_QMARK + 1`** rows). **Elevation** is **`NUM_SPECIES`** bytes (`[NUM_SPECIES]`). All are pulled into **`src/data.c`** / **`src_transformed/data.c`**. Battle, trade, daycare, Pokémon Jump, and animation code index them via **`include/data.h`** macros **`gMonFrontPicCoords`**, **`gMonBackPicCoords`**, **`gEnemyMonElevation`**.

Portable access: **`gMonFrontPicCoordsActive`**, **`gMonBackPicCoordsActive`**, **`gEnemyMonElevationActive`**. When all three host mirrors are installed by refresh, the macros resolve to those pools; otherwise the compiled tables are used (**all-or-nothing** for the pack).

## Pack (`FIRERED_ROM_MON_PIC_LAYOUT_PACK_OFF`)

Env hex + optional profile row **`__firered_builtin_mon_pic_layout_profile_none__`**.

| Offset | Size | Field |
|--------|------|--------|
| **0** | **4** | Magic **`0x4349504D`** (`MPIC` LE) |
| **4** | **4** | Version **`1`** |
| **8** | **4** | **`coord_entry_count`** — must equal **`SPECIES_UNOWN_QMARK + 1`** |
| **12** | **4** | **`elev_entry_count`** — must equal **`NUM_SPECIES`** |
| **16** | **4** | **`mon_coords_row_bytes`** — must equal **`sizeof(struct MonCoords)`** (**2**) |
| **20** | **4** | **`reserved`** — must be **`0`** |
| **24** | **`coord_entry_count × row_bytes`** | Front pic coords, row-major |
| *follows* | **`coord_entry_count × row_bytes`** | Back pic coords |
| *follows* | **`elev_entry_count × 1`** | Enemy elevation bytes |

**`struct MonCoords`** wire is the decomp layout: packed **`u8 size`** (width/height in **`MON_COORDS_SIZE`** nibbles) + **`u8 y_offset`**.

**Validation:** **`reserved == 0`**; counts match **`SPECIES_UNOWN_QMARK + 1`** / **`NUM_SPECIES`**; **`mon_coords_row_bytes == 2`**; each coord row has **non-zero** width/height nibbles **≤ 15** (matches **`GET_MON_COORDS_WIDTH` / `GET_MON_COORDS_HEIGHT`** usage). On any failure, pools are freed and all three **`*Active`** pointers are **NULL**.

**Refresh:** **`firered_portable_rom_mon_pic_layout_refresh_after_rom_load()`** from **`engine_backend_init`**, immediately after the species info table refresh.
