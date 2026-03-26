# Portable ROM: wild encounter family

Runtime seam for **`gWildMonHeaders`** and its **pointer graph** (land / water / rock smash / fishing) on **PORTABLE** builds. **Implemented:** **`FIRERED_ROM_WILD_ENCOUNTER_FAMILY_PACK_OFF`** → **`firered_portable_rom_wild_encounter_family_refresh_after_rom_load()`** (from **`engine_backend_init`**, with other ROM refresh hooks) → **`FireredWildMonHeadersTable()`** used by **`src/wild_encounter.c`** and **`src/wild_pokemon_area.c`**. When inactive or invalid, the game keeps **compiled** **`gWildMonHeaders`** from **`wild_encounters.h`**.

For planning / inspection workflows, still read **`docs/generated_data_rom_seam_playbook.md`** §2 and §7–§8, and **`docs/rom_blob_inspection_playbook.md`** for **`tools/rom_blob_inspect.py`**.

## 1. Data structure summary (wire mental model)

**Headers (index / key):** **`gWildMonHeaders[]`** — canonical generated source: **`pokefirered_core/generated/src/data/wild_encounters.h`**. Each row is **`struct WildPokemonHeader`** (**`include/wild_encounter.h`**):

| Field | Role |
|--------|------|
| **`mapGroup`**, **`mapNum`** | Map identity for this header row. |
| **`landMonsInfo`**, **`waterMonsInfo`**, **`rockSmashMonsInfo`**, **`fishingMonsInfo`** | **Optional** pointers to **`struct WildPokemonInfo`**; **NULL** means that encounter class is absent for the map. |

**Per-class info:** **`struct WildPokemonInfo`**

| Field | Role |
|--------|------|
| **`encounterRate`** | **`u8`** rate for that class. |
| **`wildPokemon`** | Pointer to a **`struct WildPokemon`** array (the slot table for that class). |

**Slots:** **`struct WildPokemon`** — **`u8 minLevel`**, **`u8 maxLevel`**, **`u16 species`**.

**Fixed slot counts** (validate row lengths against these, not guesses):

| Constant | Value | Typical use |
|----------|-------|-------------|
| **`LAND_WILD_COUNT`** | 12 | Grass / land tables |
| **`WATER_WILD_COUNT`** | 5 | Surf |
| **`ROCK_WILD_COUNT`** | 5 | Rock Smash |
| **`FISH_WILD_COUNT`** | 10 | Fishing rod bundle |

**Related generated surface:** encounter-chance constants and table bodies live in the same **`wild_encounters.h`** tree; hacks that repack or resize tables change **pointer topology** and **row counts** relative to vanilla—portable ROM seams must match **your** sym/map layout, not an assumed retail layout.

**Altering Cave:** **`NUM_ALTERING_CAVE_TABLES`** (**9**) in **`wild_encounter.h`** is part of the broader wild ecosystem; any ROM plan that touches altering-cave tables must keep **indices and pointers** coherent with whatever the engine expects for that fork.

## 2. Pointer closure checklist (before treating a slice as “validated”)

Use this after you have a **file offset** into the **mapped `.gba`** (same convention as other **`docs/portable_rom_*.md`** peers).

1. **Header table** — Confirm the **starting offset** and **byte length** for the **`gWildMonHeaders`** array (or the subset you intend to back); length must match **`N * sizeof(struct WildPokemonHeader)`** for the agreed row count **`N`**, using **`sizeof` from a portable build**, not hand-waved padding.
2. **Per-row pointers** — For each populated **`WildPokemonInfo*`**, the stored word must be a plausible **GBA ROM pointer** (**`0x08……`**) into **this** ROM image (or an explicitly documented overlay window). **NULL** is **`0x00000000`** in rodata.
3. **`WildPokemonInfo` targets** — Each non-NULL info struct: **`encounterRate`** is a **`u8`**; the following pointer targets a **`WildPokemon`** array whose **length** matches the **class** (land **12**, water **5**, rock **5**, fish **10**) for vanilla-shaped data—**forks may differ**; validate against your map/sym, not only comments here.
4. **`WildPokemon` arrays** — Slot structs are **4-byte-aligned rows** in the usual layout (**`sizeof(struct WildPokemon)`** is **4** on typical GBA/portable layouts—still verify). **Species** IDs and level ranges should match game data expectations for sanity; out-of-range values are a sign of wrong offset or wrong stride.
5. **Transitive closure** — Every pointer reachable from the header table must resolve to bytes **inside the same coherent ROM plan** (single image, or documented multi-region load). **No dangling half-migrated** pointers into compiled-only memory.
6. **Companion consistency** — If you ever split wild data across **multiple** ROM blobs (e.g. headers in one slice, slot arrays in another), **all slices that share implicit ordering or IDs** must activate **together** with the same indexing contract (mirror **`docs/portable_rom_heal_locations_table.md`** whiteout **pair** rule).

## 3. Runtime pack (**`FIRERED_ROM_WILD_ENCOUNTER_FAMILY_PACK_OFF`**)

Portable builds may load a **single coherent pack** (env hex **file** offset, then optional profile rows in **`firered_portable_rom_wild_encounter_family.c`**) that replaces **`gWildMonHeaders`** and **all** referenced **`WildPokemonInfo` / `WildPokemon`** graphs for that session when validation succeeds. Otherwise the engine keeps compiled **`gWildMonHeaders`**.

| Offset (relative to pack base) | Size | Content |
|--------------------------------|------|---------|
| **+0** | **4** | Magic **`0x464E4957`** (`WINF` LE) |
| **+4** | **4** | Version **`1`** (LE **`u32`**) |
| **+8** | **4** | **`header_bytes`** (LE **`u32`**) — length of the following GBA-wire header blob |
| **+12** | **`header_bytes`** | GBA **`WildPokemonHeader` wire rows**, **20 bytes** each: **`mapGroup`**, **`mapNum`**, **2 padding bytes (must be zero)**, four LE **`u32`** GBA ROM pointers (**`0x08……`** or zero) for land / water / rock smash / fishing. **Last row** must be the **`MAP_UNDEFINED`** sentinel (**`MAP_GROUP` / `MAP_NUM`** only; pointers zero). The sentinel must be the **final** row (no rows after it). |

Downstream structs in ROM must match **`LAND_WILD_COUNT` / `WATER_WILD_COUNT` / `ROCK_WILD_COUNT` / `FISH_WILD_COUNT`** slot counts. **`firered_portable_rom_wild_encounter_family_refresh_after_rom_load()`** runs from **`engine_backend_init`** with the other portable ROM table refresh hooks (see **`engine/core/engine_runtime_backend.c`**).

**Offline structural check:** **`tools/portable_generators/validate_wild_encounter_family_pack.py`** on a ROM file (**`--pack-offset`** = **`FIRERED_ROM_WILD_ENCOUNTER_FAMILY_PACK_OFF`**) or on a standalone pack image starting with **`WINF`**.

**Pack builder:** **`tools/portable_generators/build_wild_encounter_family_pack.py`** reads **`src/data/wild_encounters.json`** (FireRed vs LeafGreen via **`--edition`**) and **`include/constants/map_groups.h`** / **`include/constants/species.h`** for map and species ids. Emits **`WINF`** v1 with the same 20-byte header rows, **`WildPokemonInfo`** (**8** bytes: rate + **3** zero pads + LE mon pointer), and slot tables (**`LAND_WILD_COUNT` / …** rows). **`--file-offset-base P`** sets **`0x08000000 + P + offset_in_pack`** for every pointer (use when the pack is embedded at file offset **`P`** in a full **`.gba`**; default **`P=0`** matches a standalone **`.bin`** smoke test against the validator).

**Legacy / inspect-only name:** **`FIRERED_ROM_WILD_MON_HEADERS_TABLE_OFF`** — raw **`gWildMonHeaders[]`** rodata in a linked ROM (pointer-heavy); use **`rom_blob_inspect.py`** and sym/map proof before treating offsets as authoritative.

## 4. Validation bullets (read-only)

- **Bounds:** Slice **`[offset, offset + length)`** must lie inside the ROM file size; **`tools/rom_blob_inspect.py`** errors if not.
- **Exact length:** **`--expect-len BYTES`** — set **`BYTES`** to the agreed total for the slice under test (headers: **`N * sizeof(struct WildPokemonHeader)`**; a pure **`WildPokemon`** run: **`slots * 4`** if **`sizeof(struct WildPokemon) == 4`**).
- **Stride guard:** **`--struct-stride BYTES`** — require **`length % BYTES == 0`**. Use the stride that matches the struct or column you are inspecting (**`sizeof(struct WildPokemonHeader)`** for full header rows, **`4`** for pure **`u32`** pointer columns or **`WildPokemon`** rows if verified).
- **Pointer-shaped words:** **`--gba-u32-pointers`** — after the u32 summary, classifies words as zero / in-image **`0x08……`** / outside image / other. Use on slices that are **mostly or entirely `u32`** (e.g. pointer columns extracted conceptually, or header rows where you are focusing on pointer words—note mixed **`u8`**/**`u32`** layouts may need **per-region** slices for clean interpretation).
- **Cross-check counts:** Row counts for **`gWildMonHeaders`** and per-class slot counts must match **sym/map** and **`wild_encounters.h`**, not an assumed vanilla count.
- **Sym/map first:** Obtain offsets from the **built** ROM’s map file (or equivalent) for **this** fork; do not copy offsets from another hack.

**Example inspect lines** (replace paths, offsets, and lengths with **your** values):

```text
python tools/rom_blob_inspect.py path\to\game.gba -o 0xWILD_HEADERS_OFF -n 0xBYTES --expect-len 0xBYTES --struct-stride 20 --gba-u32-pointers
```

```text
python tools/rom_blob_inspect.py path\to\game.gba -o 0xPURE_U32_REGION_OFF -n 0xBYTES --expect-len 0xBYTES --struct-stride 4 --gba-u32-pointers
```

**Stride note:** **`20`** for **`struct WildPokemonHeader`** is typical on GBA-aligned builds (**2×`u8`** + **padding to 4** + **4×`u32`** pointers)—**confirm with `sizeof(struct WildPokemonHeader)`** on your portable toolchain before locking documentation.

## 5. Explicit “do not half-migrate” list

- **Do not** ROM-back **`gWildMonHeaders`** while leaving **some** of its **`WildPokemonInfo` / `WildPokemon`** targets in **compiled-only** rodata unless every pointer is explicitly redirected in a **fully documented** layout (no mixed NULL/non-NULL confusion across sources).
- **Do not** mix **ROM** header rows with **compiled** sub-tables for **the same** map’s land/water/rock/fish **inconsistently** (e.g. ROM land + compiled water for the same logical map key).
- **Do not** change **header table length** or **map group/num** ordering without updating **every** consumer that indexes wild data the same way (including fork-specific tables).
- **Do not** validate only **one** pointer column family (e.g. land only) and assume the rest of the graph is sound.
- **Do not** treat **`cores/firered/generated/data/*`** wrapper **`#include`** shims as the **canonical** byte source for audits—use **`pokefirered_core/generated`** paths for **content** (same pattern as **`docs/portable_rom_map_generated_data_playbook.md`**).
- **Do not** use this doc as an excuse for **speculative** edits to **`overworld.c`**, **`pokefirered_core/generated/src/data/map_data_portable.c`**, **`fieldmap.c`**, or **`portable_script_pointer_resolver.c`** without sym/map proof—prefer **`rom_blob_inspect.py`** first (**`docs/generated_data_rom_seam_playbook.md`** §7). Wild family wiring in **`wild_encounter.c`**, **`wild_pokemon_area.c`**, and the portable refresh hook is **intentional** once a pack layout is locked.

## 6. Related docs

| Doc | Use |
|-----|-----|
| **`docs/generated_data_rom_seam_playbook.md`** | Wild seam overview, coherent blob checklist, pointer-free vs pointer-heavy guidance. |
| **`docs/portable_rom_map_generated_data_playbook.md`** | Map-generated data context, env naming pattern, **`rom_blob_inspect.py`** entry points. |
| **`docs/rom_blob_inspection_playbook.md`** | Full flag reference and offset conventions. |
| **`include/wild_encounter.h`** | Struct definitions and **`LAND_WILD_COUNT`** / **`WATER_WILD_COUNT`** / **`ROCK_WILD_COUNT`** / **`FISH_WILD_COUNT`**. |
| **`tools/portable_generators/build_wild_encounter_family_pack.py`** | Emit **`WINF`** from **`wild_encounters.json`** + constants. |
| **`tools/portable_generators/validate_wild_encounter_family_pack.py`** | Offline structural validation. |
