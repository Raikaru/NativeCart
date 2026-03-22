# Portable ROM: level-up learnsets family (`gLevelUpLearnsets`)

Portable builds compile the pointer table as **`gLevelUpLearnsets_Compiled`** (`src/data/pokemon/level_up_learnset_pointers.h`). Each entry is a **`const u16 *`** to a **little-endian** packed stream:

- **`LEVEL_UP_MOVE(level, move)`** → **`(level << 9) | move`** (see `src/data/pokemon/level_up_learnsets.h`).
- **`LEVEL_UP_END`** → **`0xFFFF`** (must appear exactly once per species list, terminating that list).

Consumers (`pokemon.c` / `src_transformed/pokemon.c`) index **`gLevelUpLearnsets[species]`** for **`species < NUM_SPECIES`**.

## Pack contract (`FIRERED_ROM_LEVEL_UP_LEARNSETS_PACK_OFF`)

Single **coherent** pack at a **file offset** into the mapped ROM (env hex, then optional profile rows — **`__firered_builtin_level_up_learnsets_profile_none__`**). All offsets below are **bytes from the start of the pack** (not from ROM 0).

| Offset | Size | Field |
|--------|------|--------|
| **0** | **4** | Magic **`0x314C554C`** (`LUL1` LE) |
| **4** | **4** | Version **`1`** (`u32` LE) |
| **8** | **4** | **`num_species`** — must equal **`NUM_SPECIES`** for this build |
| **12** | **4** | **`ptr_table_off`** — start of pointer table |
| **16** | **4** | **`blob_off`** — start of **`u16`** payload blob |
| **20** | **4** | **`blob_len`** — byte length of blob (even, **> 0**) |

**Pointer table** (starts at **`ptr_table_off`**):

- **`num_species × 4`** bytes, **`u32` LE** per species index **`0 .. num_species - 1`**.
- Word **`rel_off`** is a **byte offset into the blob** (from **`blob_off`**’s logical start after copy). Must be **2-byte aligned** and **< blob_len**.
- Each species list is read as **`u16`** LE from **`blob[rel_off .. blob_len)`** until **`LEVEL_UP_END`**.

**Layout rules**

1. **`ptr_table_off` ≥ 24** and **`blob_off` ≥ 24** (no overlap with the 24-byte header).
2. Pointer table region **`[ptr_table_off, ptr_table_off + num_species×4)`** and blob region **`[blob_off, blob_off + blob_len)`** must **not overlap**.
3. Both regions must lie entirely in the ROM image together with the pack base: **`pack_off + region_end ≤ rom_size`**.

## Runtime validation (activation is all-or-nothing)

**`firered_portable_rom_level_up_learnsets_family.c`** copies the blob into host memory, validates **every** species, then installs **`NUM_SPECIES`** row pointers into that copy. If **any** check fails, the module frees buffers and **`FireredLevelUpLearnsetsTable()`** falls back to **`gLevelUpLearnsets_Compiled`** (no mixed ROM/compiled rows).

Per-list checks:

- Within **`blob_len`**, find **`LEVEL_UP_END`** before exceeding **2048** encoded words (guardrail).
- For each non-terminator word: level nibble field **`(w & LEVEL_UP_MOVE_LV) >> 9`** must be **`1 .. MAX_LEVEL`**; move **`(w & LEVEL_UP_MOVE_ID)`** must be **`< MOVES_COUNT`**.

## Access path

- **`include/pokemon.h`**: **`#define gLevelUpLearnsets (FireredLevelUpLearnsetsTable())`** when **`PORTABLE`**, else **`gLevelUpLearnsets_Compiled`**.
- **`FireredLevelUpLearnsetsTable()`** is implemented in the portable module.
- **Refresh:** **`firered_portable_rom_level_up_learnsets_family_refresh_after_rom_load()`** from **`engine_backend_init`** (after TM/HM learnsets, before egg moves).

## Tooling note (native GBA ROM)

Retail ROMs store **bus addresses** (`0x08……`) in the pointer table. This pack uses **blob-relative offsets** so a **single contiguous dump** can be validated without remapping GBA addresses — generate packs with a small tool that walks decomp sym/map or a known layout.
