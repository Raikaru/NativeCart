# ROM-backed heal / fly locations (`sHealLocations`, PORTABLE)

## Purpose

**`sHealLocations[]`** (see **`src/data/heal_locations.h`**, generated from **`heal_locations.json`**) stores **per-heal-location** map coordinates used for **fly destinations** and related lookups (`GetHealLocation`, `GetHealLocationIndexFromMapGroupAndNum`). This is **map-adjacent generated data** (not battle move stats): hacks that **move Pokémon Centers or fly points** can mirror the retail ROM layout when it matches this build’s **`struct HealLocation`** row count.

**Whiteout companions:** **`sWhiteoutRespawnHealCenterMapIdxs`** / **`sWhiteoutRespawnHealerNpcIds`** (same row order as heal ids **`1 … HEAL_LOCATION_COUNT`**) can be ROM-backed **only as a pair**: both env offsets must resolve, fit in ROM, and validate, or **both** fall back to compiled **`*_Compiled`** (macros in **`heal_location.c`**). This avoids mixing ROM map idxs with compiled NPC ids (or vice versa).

## Layout contract (wire format)

### Heal / fly (`sHealLocations`)

- **Rows:** **`HEAL_LOCATION_COUNT`** (20 on vanilla FireRed), same order as compiled **`sHealLocations_Compiled[]`** / heal location ids **`1 … HEAL_LOCATION_COUNT`**.
- **Row size:** **`sizeof(struct HealLocation)`** (typically **6** bytes on portable GCC: **`s8 mapGroup`**, **`s8 mapNum`**, **`s16 x`**, **`s16 y`** — verify with `sizeof` before assuming wire dumps).
- **Total blob:** **`HEAL_LOCATION_COUNT * sizeof(struct HealLocation)`** bytes.

### Whiteout heal-center map idxs (`sWhiteoutRespawnHealCenterMapIdxs`)

- **Rows:** **`HEAL_LOCATION_COUNT`**, **little-endian** **`u16` mapGroup**, **`u16` mapNum** per row (same semantic ranges as the heal blob’s map fields).
- **Total blob:** **`HEAL_LOCATION_COUNT * 4`** bytes (**80** when count is 20).

### Whiteout nurse / Mom local ids (`sWhiteoutRespawnHealerNpcIds`)

- **Rows:** **`HEAL_LOCATION_COUNT`** **`u8`** local object ids (non-zero in validation).
- **Total blob:** **`HEAL_LOCATION_COUNT`** bytes (**20** when count is 20).

## Validation (fail-safe)

- **Heal:** Light range checks on **map group/num** and **x/y**; on failure **`gHealLocationsActive`** is **NULL** (compiled **`sHealLocations_Compiled`**).
- **Whiteout:** Map idx rows use the same **group/num** ranges as heal; NPC row rejects **0**. If either whiteout blob fails, **`gWhiteoutRespawnHealCenterMapIdxsActive`** and **`gWhiteoutRespawnHealerNpcIdsActive`** are both **NULL** (compiled **`*_Compiled`**).

## Runtime wiring

- **Env / profile:** **`FIRERED_ROM_HEAL_LOCATIONS_TABLE_OFF`**, **`FIRERED_ROM_WHITEOUT_HEAL_CENTER_MAP_IDXS_OFF`**, **`FIRERED_ROM_WHITEOUT_HEALER_NPC_IDS_OFF`** (+ placeholder profile id **`__firered_builtin_heal_locations_profile_none__`** on each resolver).
- **Refresh:** **`firered_portable_rom_heal_locations_table_refresh_after_rom_load()`** from **`engine_runtime_backend.c`** (after tutor tables).
- **Access:** **`gHealLocationsActive`**; **`gWhiteoutRespawnHealCenterMapIdxsActive`** / **`gWhiteoutRespawnHealerNpcIdsActive`** in **`heal_location.h`**; **`#define sHealLocations`** / whiteout **`#define`s** in **`heal_location.c`** / **`src_transformed/heal_location.c`**. Loops use **`HEAL_LOCATION_COUNT`** from **`constants/heal_locations.h`** (not **`ARRAY_COUNT`** on the macro).

## Non-portable builds

Unchanged **`sHealLocations`** symbol; no ROM path.
