# ROM-backed region map fly destinations (`sMapFlyDestinations`, PORTABLE)

## Purpose

The fly map uses **`sMapFlyDestinations[mapsec - KANTO_MAPSEC_START][3]`**: **`u8` map group**, **`u8` map num** (see **`MAP()`** in **`constants/maps.h`**), and **`u8` heal location id** (or **`HEAL_LOCATION_NONE`**). See **`SetFlyWarpDestination`** in **`src/region_map.c`**.

This ties **ROM-backed heal locations** to **map-adjacent** fly behavior without pointer indirection.

## Layout contract

- **Rows:** **`REGION_MAP_FLY_DESTINATION_ROW_COUNT`** (= **`MAPSEC_EMBER_SPA - KANTO_MAPSEC_START + 1`**, see **`include/constants/region_map_fly_destinations.h`**).
- **Row size:** **3** bytes (`group`, `num`, `heal`).
- **Total:** **`REGION_MAP_FLY_DESTINATION_ROW_COUNT × 3`** bytes.

## Validation (fail-safe)

- Map **group** **0…50**, **num** **0…120** (same coarse bounds as other portable map slices).
- **Heal** byte **0** or **1…`HEAL_LOCATION_COUNT`**.

On failure, **`gMapFlyDestinationsActive`** stays **NULL**; **`region_map.c`** uses **`sMapFlyDestinations_Compiled`**.

## Runtime wiring

- **Env / profile:** **`FIRERED_ROM_REGION_MAP_FLY_DESTINATIONS_TABLE_OFF`** (+ **`__firered_builtin_region_map_fly_destinations_profile_none__`**).
- **Refresh:** **`firered_portable_rom_region_map_fly_destinations_refresh_after_rom_load()`** from **`engine_runtime_backend.c`** (after section layout).
- **Access:** **`gMapFlyDestinationsActive`** in **`region_map_fly_destinations_rom.h`**; **`#define sMapFlyDestinations`** in **`region_map.c`**.

## Non-portable builds

Unchanged **`sMapFlyDestinations`** symbol; no ROM path.
