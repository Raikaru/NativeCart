# Generated data / map / layout ROM seam (analysis playbook)

Use this when planning the **next ROM-backed slice** for data that still lives in **decomp-generated C/H** (maps, wild tables, region metadata), without touching runtime chokepoints. Prefer **read-only** inspection first: `tools/rom_blob_inspect.py` (see `docs/rom_blob_inspection_playbook.md`) and `tools/list_generated_data_candidates.py`. For a **dense, pointer-free** table (no `0x08……` columns), walk **§8** before writing env docs or touching merge-hot C.

Complements **`docs/next_rom_structural_seam_notes.md`** (battle/learnset/item-style flat blobs).

## 1. Where generated files live (and how they relate)

| Location | Role |
|----------|------|
| **`pokefirered_core/generated/`** | **Canonical tree** checked into the repo: portable data C sources, headers, constants (`src/data/*`, `include/`, etc.). |
| **`cores/firered/generated/data/*.c`** | **Thin wrappers**: each file is typically a single `#include` back to the matching path under `pokefirered_core/generated/src/data/…`. Example: `cores/firered/generated/data/map_data_portable.c` includes `../../../../pokefirered_core/generated/src/data/map_data_portable.c`. |

So: **edit or audit the real content in `pokefirered_core/generated`**; the core wrapper directory exists so the firered core build can compile the same sources without duplicating megabytes of text.

## 2. Wild encounter pointers (likely seam shape)

Source of truth in-tree: **`pokefirered_core/generated/src/data/wild_encounters.h`**.

- **`gWildMonHeaders[]`** — `struct WildPokemonHeader` (`include/wild_encounter.h`): `u8` map group/num, then **four pointers** to optional `WildPokemonInfo` structs (land / water / rock smash / fishing). In a vanilla ROM image this is a **pointer-heavy** region: follow **`0x08……`** targets only after confirming they land **inside the ROM file** (same rules as other overlay tables).
- **`WildPokemonInfo`** — encounter rate + pointer to a **`WildPokemon`** array (per-slot `u8` min/max level + `u16` species).
- **ROM analysis workflow** (no engine edits): locate the **header table** and each **sub-table** in the sym/map; slice with `rom_blob_inspect.py`; use **`--struct-stride 4`** on pure `u32` regions and **`--gba-u32-pointers`** to classify words; cross-check **row counts** (e.g. land **12**, water **5**, fishing **10** slots) against `LAND_WILD_COUNT` / `WATER_WILD_COUNT` / `FISH_WILD_COUNT` in `include/wild_encounter.h`.

Forks that add maps or repack wild data break portable builds when **counts or pointer topology** no longer match the compiled headers—overlays cannot fix that without a matching layout spec.

**Reference:** **`docs/portable_rom_wild_encounter_family.md`** — data layout summary, pointer-closure checklist, suggested **`FIRERED_ROM_*`** placeholders, **`rom_blob_inspect.py`** notes, and an explicit do-not-half-migrate list.

## 3. `map_data_portable.c` scale warning

**`pokefirered_core/generated/src/data/map_data_portable.c`** aggregates a **very large** amount of map layout and related data (on the order of **millions of characters / hundreds of thousands of lines**).

Implications for **analysis** (not necessarily for the compiler):

- Full-file open in some editors is slow; prefer **`tools/list_generated_data_candidates.py`**, targeted **`grep`/`rg`**, or reading **head/tail** slices.
- Treat **full regeneration or manual merge** of this file as a **high-cost** operation; for a **first ROM seam**, prefer a **smaller, bounded** table (constants, a single map family, or wild headers) to prove offset + validation before attacking the monolithic map blob.

## 4. Suggested order for a **first slice**

1. **Smallest pointer-stable surface** — e.g. a **fixed-count** table already documented elsewhere, or a **small generated header** under `pokefirered_core/generated/include/constants/` if the goal is to validate toolchain + `rom_blob_inspect.py` on real offsets.
2. **`gWildMonHeaders` + one encounter family** — bounded struct sizes, clear pointer semantics, easy to cross-check against `wild_encounter.h`.
3. **Region map entries / strings** (under `pokefirered_core/generated/src/data/region_map/`) — smaller than full `map_data_portable.c`, still “layout-ish.”
4. **`map_data_portable.c` / full map graph** — only after offsets and validation patterns are reliable; consider incremental hacks (single map group) rather than whole-file ROM backing.

Throughout: **do not** route first experiments through `engine_runtime_backend.c`, `fieldmap.c`, `src_transformed/pokemon.c`, or `portable_script_pointer_resolver.c` unless the task explicitly requires it—those are **merge and regression hotspots**.

## 5. `rom_blob_inspect.py` reminders for generated-data seams

Existing flags usually suffice; no new flags are required for basic checks:

- **`--expect-len BYTES`** — exact byte length of the slice (for a **`u16[N]`** blob, pass **`2 * N`**).
- **`--struct-stride BYTES`** — alignment guard (e.g. multiple of **4** for `u32` pointer tables).
- **`--gba-u32-pointers`** — sanity-check that words look like in-ROM **`0x08……`** pointers (wild encounter header graphs: **`docs/portable_rom_wild_encounter_family.md`** §4).

## 6. Tool: `tools/list_generated_data_candidates.py`

Read-only scan of **`pokefirered_core/generated`**: lists **`.c` / `.h`** files with **line counts**, sorted by size so large seam targets (map data, wild tables, script blobs) show up immediately.

```text
python tools/list_generated_data_candidates.py
python tools/list_generated_data_candidates.py --min-lines 5000
python tools/list_generated_data_candidates.py --root pokefirered_core/generated/src/data
python tools/list_generated_data_candidates.py --grep wild
python tools/list_generated_data_candidates.py --grep map --json
```

Use the output to choose **what to diff against a ROM** and what to open with care (large files first).

## 7. Coherent blob checklist (safe migration shape)

A **coherent blob** is a slice you can reason about without spelunking the whole engine: fixed layout, bounded size, and explicit relationships to the rest of the data ROM. Use this before moving bytes from `pokefirered_core/generated` to a ROM-backed table or overlay.

### Checklist

1. **One index contract** — There is a single authoritative row order (array index, ID minus base, or map group/num key). Every consumer agrees on how a row is found.
2. **Companion tables stay aligned** — If two or more arrays are indexed the same way (see example below), **row count and ordering** must move together; partial migration of one column family without the others is a common breakage.
3. **Dimensions match the grid** — Constants like map counts, heal location counts, or header table lengths must match the **actual number of populated entries** you ship. See anti-patterns.
4. **Struct stride and endianness** — `sizeof`/padding matches the decomp; use `rom_blob_inspect.py` with `--struct-stride` / `--expect-len` where applicable.
5. **Pointer closure** — If the blob holds ARM7 `0x08……` words, every target is either inside the same ROM image or explicitly documented as an overlay/load region before you treat the slice as validated.
6. **Read-only first** — `tools/rom_blob_inspect.py`, sym/map cross-checks, and this repo’s playbooks; avoid threading experiments through `engine_runtime_backend.c`, `fieldmap.c`, or `portable_script_pointer_resolver.c` unless the task demands it.

### Example: heal / whiteout companion tables

FireRed keeps **three parallel tables** derived from the same heal-location list: fly/checkpoint positions, whiteout respawn map (group/num), and the NPC id on that map for the hand-off cutscene. The generator template shows the intended lockstep indexing (`[id - 1]` on each array):

- `src/data/heal_locations.json.txt` — `sHealLocations`, `sWhiteoutRespawnHealCenterMapIdxs`, `sWhiteoutRespawnHealerNpcIds`

Portable wiring splits this into **independent** heal vs whiteout ROM paths: **`FIRERED_ROM_HEAL_LOCATIONS_TABLE_OFF`** can succeed or fail on its own. The **two** whiteout tables (**`FIRERED_ROM_WHITEOUT_HEAL_CENTER_MAP_IDXS_OFF`** + **`FIRERED_ROM_WHITEOUT_HEALER_NPC_IDS_OFF`**) activate **only as a pair** (same `HEAL_LOCATION_COUNT`, same row order) so map idxs and NPC ids never mix ROM vs compiled. See **`docs/portable_rom_heal_locations_table.md`**.

### Anti-patterns

- **Dimensions without the full grid** — e.g. bumping a `NUM_*` or header count while only embedding a subset of rows; the engine will index past valid data or read garbage.
- **Splitting companion arrays across sources** — e.g. ROM-backed `sHealLocations` but still compiling old `sWhiteoutRespawnHealCenterMapIdxs` from C with a different order or length.
- **Pointer columns before targets** — migrating pointer tables when the pointed-to blob is not yet in the same coherent ROM layout.
- **Implicit coupling to map or event data** — heal NPC ids and respawn maps must still match **live** map headers and object events; changing the table alone without validating map JSON / generated events is not coherent.

## 8. Next **pointer-free** ROM table: pre-flight validation sheet

Use this when the seam is a **flat byte run** (fixed row count × known wire stride) rather than a pointer vector. It keeps **generated-data** work aligned with how existing `FIRERED_ROM_*` slices are documented in `docs/portable_rom_*.md` and validated at runtime—without requiring edits to `engine_runtime_backend.c`, `fieldmap.c`, `wild_pokemon_area.c`, or `portable_script_pointer_resolver.c` until you are ready to wire the actual loader.

### Checklist (copy into your design note or PR)

| Step | What to lock | Why |
|------|----------------|-----|
| 1 | **One authoritative row count** — a single macro or documented constant (`HEAL_LOCATION_COUNT`, `NUM_SPECIES`, `MOVES_COUNT`, …) shared by compiled data and ROM | Off-by-one or fork drift surfaces as out-of-bounds reads, not compile errors. |
| 2 | **Wire `sizeof` / stride** — byte length = **`rows × row_bytes`**; row_bytes come from the **struct layout doc** or a measured portable build, not hand-waving | Padding differs by toolchain; `sizeof(struct Foo)` on PORTABLE is the usual contract (see heal locations). |
| 3 | **Env var name** — follow **`FIRERED_ROM_<AREA>_<TABLE>_OFF`** (hex file offset into the mapped `.gba`) and document it next to the table in a new **`docs/portable_rom_<topic>_table.md`** | Keeps discovery consistent: `rg FIRERED_ROM_ docs` lists peers. |
| 4 | **Paired blobs** — if two ROM slices share the **same index order**, specify **both-or-neither** activation (see heal + whiteout pair, or tutor moves + learnsets in `docs/rom_backed_runtime.md`) | Prevents half-ROM / half-compiled hybrid rows. |
| 5 | **Read-only byte proof** — `rom_blob_inspect.py` with **`-n`** = expected bytes and **`--expect-len`** matching §2 | Catches wrong sym offset or truncated dump before any C wiring. |

### Example pattern (heal-sized fictional table)

Suppose a new table has **`HEAL_LOCATION_COUNT`** rows and a portable wire row of **`sizeof(struct HealLocation)`** bytes (example only—replace with your real count and struct).

1. **Expected length:** `HEAL_LOCATION_COUNT * sizeof(struct HealLocation)` (e.g. 20 × 6 = **120** when that `sizeof` is 6).
2. **Inspect** (PowerShell env for offset is optional; pass hex literals directly):

```text
python tools/rom_blob_inspect.py path\to\game.gba -o 0xYOUR_OFF -n 120 --expect-len 120 --struct-stride 6
```

3. **Discover peers** in docs: `rg "FIRERED_ROM_.*_OFF" docs` — mirror their sections (layout, validation rules, fail-open behavior).

### Worked example: region map section grid pack (2640 bytes)

Vanilla portable dims: **`REGION_MAP_SECTION_GRID_LAYERS`** × **`REGION_MAP_SECTION_GRID_HEIGHT`** × **`REGION_MAP_SECTION_GRID_WIDTH`** = **660** bytes per island group; **×4** contiguous groups (Kanto, Sevii123, Sevii45, Sevii67) = **2640** bytes at **`FIRERED_ROM_REGION_MAP_SECTION_GRIDS_PACK_OFF`**. Inspect:

```text
python tools/rom_blob_inspect.py path\to\game.gba -o 0xYOUR_OFF -n 2640 --expect-len 2640 --struct-stride 1
```

Runtime doc: **`docs/portable_rom_region_map_section_grids.md`**. **Do not** ROM-back a **subset** of the four tensors without the rest—the engine activates **all or none**.

### When this sheet does *not* apply

Pointer columns (`u32` ROM addresses), script tokens, or wild sub-tables need **§2** (wild headers) and **`--gba-u32-pointers`** / overlay docs instead; pointer closure still dominates validation.
