# Portable ROM: facility class lookup tables

Compiled data:

- **`gFacilityClassToPicIndex_Compiled[NUM_FACILITY_CLASSES]`** — facility class → front pic id (`src/data/pokemon/trainer_class_lookups.h`).
- **`gFacilityClassToTrainerClass_Compiled[NUM_FACILITY_CLASSES]`** — facility class → `TRAINER_CLASS_*` id (same header).

**`NUM_FACILITY_CLASSES`** is **`FACILITY_CLASS_CHAMPION_RIVAL_2 + 1`** (**150** in vanilla), defined in **`include/constants/trainers.h`**. Included from **`src/pokemon.c`** / **`src_transformed/pokemon.c`**. Call sites use **`gFacilityClassToPicIndex`** / **`gFacilityClassToTrainerClass`** macros from **`include/pokemon.h`** (battle tower, trainer tower, link battles, overworld sprites, etc.).

Portable access: **`gFacilityClassToPicIndexActive`** / **`gFacilityClassToTrainerClassActive`**. When **both** are non-NULL, macros use the host mirrors; otherwise compiled tables are used (**all-or-nothing**).

This seam is intentionally **not** `gTrainers` / party unions — only fixed **`u8`** rows per facility class.

## Pack (`FIRERED_ROM_FACILITY_CLASS_LOOKUPS_PACK_OFF`)

Env hex + optional profile row **`__firered_builtin_facility_class_lookups_profile_none__`**.

| Offset | Size | Field |
|--------|------|--------|
| **0** | **4** | Magic **`0x46434C43`** (`FCLC` LE) |
| **4** | **4** | Version **`1`** |
| **8** | **4** | **`entry_count`** — must equal **`NUM_FACILITY_CLASSES`** |
| **12** | **4** | **`reserved`** — must be **`0`** |
| **16** | **`entry_count`** | Pic-index column (`u8` per row) |
| *follows* | **`entry_count`** | Trainer-class column (`u8` per row) |

**Validation:** **`reserved == 0`**; **`entry_count == NUM_FACILITY_CLASSES`**; each pic byte **&lt; `TRAINER_PIC_PAINTER + 1`**; each trainer-class byte **&lt; `TRAINER_CLASS_PAINTER + 1`**. On failure, pools are freed and both **`*Active`** pointers are **NULL**.

**Refresh:** **`firered_portable_rom_facility_class_lookups_refresh_after_rom_load()`** from **`engine_backend_init`**, immediately after mon pic layout.

Offline: **`tools/portable_generators/validate_facility_class_lookups_rom_pack.py`** (`tools/docs/README_validators.md`).
