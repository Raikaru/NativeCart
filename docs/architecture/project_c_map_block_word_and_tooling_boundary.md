# Project C — Map **block word** layout & tooling boundary

**Purpose:** Pin the **on-disk / in-memory map grid word** contract and list what must move together when metatile **encoding** changes.

**Status — Phase 2 (`u16` RAM):** **PRET wire V1** on-disk (`map.bin` / `border.bin`): 10+2+4. **Phase 2 RAM** in `VMap` / `gBackupMapData` and migrated **`mapView`**: 11+2+3 (**2048** metatile ids, elevation **0..7** — vanilla FRLG maps satisfy this). Conversion: **`MapGridBlockWordFromWireV1`** (`InitBackupMapLayoutData`, **`FillConnection`**, **`GetBorderBlockAt`** / **`GetBorderBlockAtImpl`**); save migration: **`gSaveBlock1Ptr->mapGridBlockWordSaveLayout`**, **`NormalizeSavedMapViewBlockWords`**, **`SaveMapView`**. **`MAP_GRID_METATILE_ID_SPACE_SIZE`** **2048**; **`NUM_METATILES_TOTAL`** **1024** for **compiled** table sizes. Portable **`metatile_attr_word_count`** uses compiled 640/384 counts. Offline **`tools/check_project_c_block_word_offline.py`** + **`map_visual_policy_constants.py`**; layout bin / ROM pack validators use **wire V1** mask for on-disk blobs.

**Status — Phase 3 (tooling / planned `u32` on-disk):** Constants **`MAP_GRID_BLOCK_WORD_LAYOUT_PHASE3_U32`** + **`MAP_GRID_BLOCK_WORD_PHASE3_U32_*`** (16-bit metatile id, 2-bit collision, 3-bit elevation in a **`u32`** LE cell). Offline **`tools/check_project_c_phase3_offline.py`**. Validate hack bins with **`tools/validate_map_layout_block_bin.py --cell-width 4`**. **`mapjson`** still emits **`u16`** by default; **`tools/mapjson/README.md`** documents Phase 3. **Next (runtime Phase 3):** `gBackupMapData` / `VMap` / save **`mapView` as `u32`**, ROM portable loaders, coordinated migration — not started.

---

## On-disk wire format (PRET `map.bin` / `border.bin` — unchanged)

| Bits   | Field        | Constants |
|--------|--------------|-----------|
| 0–9    | Metatile id  | **`MAP_GRID_BLOCK_WORD_WIRE_V1_*`** — **10 bits**, **0..1023** |
| 10–11  | Collision    | |
| 12–15  | Elevation    | **4 bits** (vanilla FRLG data **<= 7**) |

- Emitted via **`tools/mapjson`** as `.incbin`; **validators** use **`MAP_GRID_BLOCK_WORD_WIRE_V1_METATILE_ID_MASK`** + limit **1024**.

## Phase 2 RAM format (`VMap`, `gBackupMapData`, migrated `mapView`)

| Bits   | Field        | Notes |
|--------|--------------|--------|
| 0–10   | Metatile id  | **`MAPGRID_METATILE_ID_MASK`** (0x07FF) — **11 bits**, **0..2047** |
| 11–12  | Collision    | |
| 13–15  | Elevation    | **3 bits** (**0..7**) |

- **`MapGridGetMetatileIdAt`** uses **`MAPGRID_METATILE_ID_MASK`** on **RAM** words.
- **`STATIC_ASSERT`**: **`MAP_GRID_METATILE_ID_SPACE_SIZE - 1 == MAPGRID_METATILE_ID_MASK`** (**2048** ids).

## Phase 3 on-disk `u32` cell (documented — optional hack pipeline)

| Bits    | Field        | Constants |
|---------|--------------|-----------|
| 0–15    | Metatile id  | **`MAP_GRID_BLOCK_WORD_PHASE3_U32_METATILE_ID_MASK`** — **16 bits**, **0..65535** |
| 16–17   | Collision    | **`MAP_GRID_BLOCK_WORD_PHASE3_U32_COLLISION_*`** |
| 18–20   | Elevation    | **3 bits** (**0..7**) |
| 21–31   | Reserved     | Must be **0** for forward compatibility (validators do not enforce yet) |

- **Runtime / vanilla tree:** not loaded by `fieldmap.c` yet; use **`--cell-width 2`** validators on `layouts.json`.

---

## Runtime Project C contract (already landed)

- **`MAP_GRID_METATILE_*`** — primary/secondary **row** split inside encoded space.
- **`MapGridMetatileIdIsInEncodedSpace`** — ids **&lt; `MAP_GRID_METATILE_ID_SPACE_SIZE`**.
- **Draw / previews:** invalid (non-encoded) → treat as metatile **0**; **attributes:** → **`0xFF`**.

Charter: `docs/architecture/project_c_metatile_id_map_format.md`.

---

## What a **future** Project C program (e.g. **`u32` cells / new on-disk layout**) must touch (together)

Changing **on-disk** wire layout, metatile id **width** beyond Phase 2 **`u16`**, or **primary/secondary encoding** requires coordinated work, including:

1. **`constants/map_grid_block_word.h`** / **`global.fieldmap.h`** — masks/shifts, `MAPGRID_UNDEFINED`, conversion/migration from wire V1 / Phase 2.
2. **`MapGridGetMetatileIdAt`**, **`MapGridSetMetatileIdAt`** (metatile+collision, keeps elevation), **`MapGridSetMetatileEntryAt`** (full word), **`MapGridSetMetatileImpassabilityAt`**, connection fill, and any other **`VMap.map` / `gBackupMapData`** writers — pack/unpack must stay consistent with the chosen mask layout (`global.fieldmap.h`).
3. **Save / backup map paths** — see [Save / backup touchpoints](#save--backup-touchpoints-audit) below.
4. **Map editing pipeline** — editors, exporters, and any validator that assumes **10-bit** ids; **`mapjson`** layout emission stays **incbin**-based unless a new binary schema is defined.
5. **`include/map_layout_metatiles_access.h`** — **`MAP_LAYOUT_METATILE_MAP_PTR`** / **`…_BORDER_PTR`** resolve to **`u16`** grids; ROM vs compiled paths must stay consistent if block words change shape.
6. **ROM portable packs** that embed metatile grids — directory validators and `firered_portable_rom_map_layout_metatiles.c` word counts vs **geometry** (orthogonal to id bit width but often changed in the same hack).
7. **Project B** — metatile **tile halfwords** still use **10-bit** tile indices in GBA tilemaps; widening **metatile ids** does not automatically widen those without a separate decision.

---

## Save / backup touchpoints (audit)

These paths store or restore **raw `u16` map words** with the same layout as **`VMap.map`** / layout **`map.bin`**:

| Mechanism | Location | Notes |
|-----------|----------|--------|
| **Live virtual map** | `gBackupMapData`, `VMap.map` (`fieldmap.c`) | Full in-RAM grid used by **`GetMapGridBlockAt`**; block words are **`u16`**. Populated from layout **`map.bin`** (and connection strips) via **`InitBackupMapLayoutData`** / **`FillConnection`** — raw **`CpuCopy16`** of the same format as on disk. |
| **Player map view snapshot** | `gSaveBlock2Ptr->mapView` (`include/global.h`, **`0x100`** `u16`s) | **`SaveMapView`** / **`LoadSavedMapView`** / **`ClearSavedMapView`** / **`MoveMapViewToBackup`** (`fieldmap.c`) move **`u16`** words between save RAM and **`gBackupMapData`** (Fly/return, connections). Changing **word width** or **semantic** layout implies **save struct** revision + migration. |
| **Save triggers** | `save.c`, `start_menu.c` (and transformed twins) | Call **`SaveMapView()`** before persisting; any new encoding must remain compatible with **stored `mapView` blobs** or bump save version. |
| **Layout identity in save** | **`gSaveBlock1Ptr->mapLayoutId`** (`include/global.h`) | Selects which **`MapLayout`** / ROM metatile border+map blobs apply (PORTABLE: **`FireredMapLayoutMetatile*ForLayoutId`**). Changing layout **word** format may still require **`mapLayoutId`** range / migration rules. |

**Quest log / playback:** does not appear to duplicate the full map grid in-repo under obvious `mapView`/`VMap` symbols; re-audit when adding features that **record metatile deltas** beyond vanilla.

---

## Explicit non-goals for “small Project C”

- No further **semantic** unification is required before this program: draw, attributes, and previews share **`MapGridMetatileIdIsInEncodedSpace`**.
- Another **`static inline`** in `fieldmap.h` without a **format** motivation should wait for the program above.

---

## Related docs

- `docs/architecture/README.md` — index of map-visual architecture notes.
- `docs/architecture/project_c_metatile_id_map_format.md` — metatile id **interpretation** inside encoded space.
- `docs/portable_rom_map_layout_metatiles.md` — ROM-backed layout grids and metatile **tables**.
- `tools/mapjson/` — `README.md` (layout/map **asm** generation; block binaries are opaque).
