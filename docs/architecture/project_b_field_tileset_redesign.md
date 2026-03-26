# Project B — Field map tileset redesign (architecture program)

This document **starts the larger Project B line** after the bounded contract work in `fieldmap.h`, `fieldmap.c`, `overworld.c`, `dma3_manager.c`, and `engine_renderer_backend.c`. It ranks viable directions when the game must move **beyond** the current model:

- One **contiguous** logical tile index range **[0 … N−1]** with **N ≤ 1024** (10-bit tilemap tile number).
- **Linear** VRAM addressing: `charBaseBlock × 16KiB + tileIndex × 32` (4bpp) from a **single** `charBaseIndex` on the field BG.
- **Metatile graphics** tables sized for vanilla **640 / 384** metatile splits (see `field_camera.c`, ROM layout validators).

---

## 1. Coupling points (minimum shared surface)

Any redesign must reconcile **all** of the following; skipping one produces silent desync.

| Layer | Responsibility | Hard assumption today |
|-------|----------------|------------------------|
| **Tile upload** | `Copy*TilesetToVram*`, `LoadBgTiles` / decompress | Offsets are **tile indices** × 32 into one BG char path; `MAP_BG_*` capacity |
| **DMA / VRAM** | `LoadBgVram` → `RequestDma3Copy`, `PortableDmaDestBufferRangeOk` | Destinations in **0x18000** VRAM image |
| **BGCNT / template** | `sOverworldBgTemplates`, `FieldOverworldVerifyFieldTilesetBgAgreement` | **One** `charBaseIndex` + `baseTile` for field BG |
| **Tilemap entries** | Written in `field_camera.c` (`DrawMetatile`) | **10-bit** tile number in each halfword (`attr2` mask `0x3FF`) |
| **Renderer sampling** | `engine_render_4bpp_tile`, text BG pixel path | Same linear formula from **current** `BGCNT` char base |
| **Metatile → tiles** | `metatiles + metatileId * 8` u16 stream | Indices are **logical** map tile IDs (0…1023 today) |
| **Metatile ID map grid** | `NUM_METATILES_IN_PRIMARY` / `NUM_METATILES_TOTAL` | **Project C** if grid semantics change |
| **Offline validators** | e.g. `tools/portable_generators/validate_map_layout_metatile_graphics_directory.py` | Default **640×8 / 384×8** halfwords per metatile row |

**Hardest blockers:** (1) **10-bit** tilemap encoding on real GBA text BGs without changing **DISPCNT/mode** or **tilemap format**; (2) **metatile** streams and **map data** if logical IDs widen; (3) keeping **portable** and **GBA** paths behavior-matched.

---

## 2. Redesign directions (ranked)

### D — Strict capped contract + explicit rejection (policy-only extension) — **charter complete**

**Idea:** Keep **one contiguous** linear space; raise caps only within **hardware-safe** bounds (still ≤ 1024 tile numbers, ≤ 2×16KiB span from one char base, VRAM/DMA checks). Reject ROM packs that exceed policy at **validation / load** time.

- **Leverage:** Smallest code churn; extends current `MAP_BG_*` + asserts + DMA guards.
- **Complexity:** Low.
- **Write set:** `fieldmap.h`, validators, optional ROM refresh errors.
- **Risks:** Does **not** solve “need more than 1024 **visible** tile numbers” or non-contiguous art packing.

**Landed (this repo, “Direction D done”):** shared **`map_visual_policy_constants.py`**; ROM / layout validators with **`MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES`** and opt-in larger metatile blobs; **`--enforce-role-tile-caps`** on **`validate_compiled_tileset_asset_profile_directory.py`** and **`validate_compiled_tileset_tiles_directory.py`** (**`compiled_tileset_table_roles.py`**); debug **`AGB_ASSERT`** on physical TID in **`FieldMapTranslateMetatileTileHalfwordForFieldBg`**; offline **`tools/check_direction_d_offline.py`** + **`make check-direction-d`** / CI / **`verify_portable_default.*`**. Further strict-cap work is **per-pack** validator flags — not “more D.” **Bounded Direction B** (logical→physical) is **§5**; **larger B** or **Lane A/C** layout changes are separate programs.

**Rank:** **#1 for immediate safety** and for **hack discipline**; **not** a path to >1024 tile **numbers** in tilemaps.

---

### B — Logical tile-address mapping / indirection (software translation) — **bounded charter complete**

**Idea:** Keep **tilemap** entries in a **stable logical ID** space (still 10-bit on hardware **unless** tilemap format changes). Introduce a **translation** layer: `logicalId → {charBlock, tileIndex, palette…}` or `logicalId → offsetInCombinedBuffer` before upload **and** at **sample** time (portable renderer), or pre-expand tilemaps when building ROMs.

- **Leverage:** Decouples **packing** in ROM from **hardware** tile numbers; can support **banks** without changing metatile **count** semantics immediately if translation is centralized.
- **Complexity:** **High** — must be applied at **every** consumer of map tile numbers (draw, animations, effects that patch tilemaps, Teachy TV, doors, etc.).
- **Write set:** New shared module; `field_camera`; renderer path that decodes tilemap halfwords; possibly code generators.
- **Risks:** Performance (portable), bugs where one path bypasses indirection; save compatibility if tilemaps in saves.

**Landed (this repo, “Direction B bounded done”):** documented **`struct FireredFieldBgTileAddressing`** in **`fieldmap.h`** (table pointer + capacity; runtime still uses **`gFieldMapLogicalToPhysicalTileId`** + inlines); **`FieldMapPhysicalTileIndexFromLogical`** / **`FieldMapTranslateMetatileTileHalfwordForFieldBg`**; **`gFieldMapLogicalToPhysicalTileId`** + identity **`.inc`**; **`field_camera`** / door path writers; **`Copy*TilesetToVram*`** upload pairing; debug **`FieldMapVerifyLogicalToPhysicalTileTable`**; **`tools/validate_field_map_logical_to_physical_inc.py`**; offline **`tools/check_direction_b_offline.py`** + **`make check-direction-b-offline`** / **`make check-direction-b`** (also first half of **`make check-validators`**); portable CI + **`verify_portable_default.*`**.

**Explicitly not in bounded scope (future / larger B):** **second** indirection in the **renderer** (tilemaps today store **physical** TIDs after draw); **versioned ROM flag** selecting an alternate addressing bundle; **>1024 logical** space without **Project C** map / metatile format work.

**Rank:** **#2** as the **main long-term** approach if the product goal is **flexible** packing or future >1024 **logical** tiles with a **format** upgrade later.

---

### A — Multi–char-base within the same BG (hardware-style banking)

**Idea:** Use **different** `charBaseIndex` values or **split** tilemaps so each screen region uses a **different** BGCNT char base — still **10-bit** indices **per** region. May require **multiple BG layers** or **dynamic** BGCNT changes per scanline (expensive / not typical for overworld).

- **Leverage:** Matches GBA **hardware** story (four 16KiB bases).
- **Complexity:** **Very high** for a **scrolling** overworld: **one** coherent tilemap is usually tied to **one** char base per BG; splitting across bases without **multiple BGs** or **affine** tricks is non-trivial.
- **Write set:** `overworld` BG setup, `field_camera`, possibly window/merge rules, renderer assumptions.
- **Risks:** Massive QA surface; easy to break priority/mosaic/text windows.

**Rank:** **#3** — powerful but **poor fit** for **single-BG2** field map unless the design **explicitly** moves to **multi-layer** map composition.

---

### C — Renderer-side virtual char plane (indirection only in engine)

**Idea:** Treat **emulated VRAM** as a **logical** 4bpp canvas for field BG only; **tilemap** indices index into this **virtual** space; **compose** pass maps to real layout. GBA-accurate path might **diverge** unless the virtual layout matches hardware.

- **Leverage:** Freedom in **portable** builds.
- **Complexity:** **High**; risks **GBA vs portable** drift.
- **Write set:** `engine_renderer_backend.c`, sync with `LoadBgVram` layout.
- **Risks:** Two truths for “where tile 417 lives.”

**Rank:** **#4** unless the project **explicitly** deprecates **cycle-accurate** VRAM for field (unlikely for this repo).

---

## 3. Recommended direction

1. **Near term (Direction D):** **Complete** for this tree — see §2 direction **D** and §4 closure notes. Optional ROM-seam hardening remains **per-pack** (run validators with **`--enforce-role-tile-caps`** when checking asset profiles / tile blobs).

2. **Deferred (post–bounded B):** Optional **versioned ROM/layout** bundle for **`FireredFieldBgTileAddressing`**, and **renderer-side** translation **only if** some path stores **logical** TIDs in tilemaps (today writers emit **physical** TIDs — no portable/GBA divergence).

### Direction B — first code step (landed; charter closure §5)

- **`fieldmap.h`:** **`FieldMapPhysicalTileIndexFromLogical`** (table lookup + **10-bit** mask) and **`FieldMapTranslateMetatileTileHalfwordForFieldBg`**, which preserves flip/palette (bits ≥10) and translates only the **10-bit TID**.
- **`src/field_map_tile_logical_to_physical.c` + `src/data/field_map_logical_to_physical_identity.inc`:** **`gFieldMapLogicalToPhysicalTileId[1024]`** — **identity** today (`[i]==i`); safe hook for codegen / ROM-specific remaps without touching every call site.
- **`field_camera.c` / `src_transformed/field_camera.c`:** **`DrawMetatile`** (and **`DrawDoorMetatileAt` → `DrawMetatile`**) routes every field tilemap halfword through the translator before writing **`gBGTilemapBuffers*`**.
- **Renderer:** still samples **physical** TIDs from tilemaps (no second indirection yet). Optional future hook in **`engine_renderer_backend.c`** only if a path can observe **untranslated** logical TIDs (field path should not).

#### Direction B — logical→physical table contract (guardrails; identity today)

**Enforced now (debug / non-`NDEBUG` builds):** `FieldMapVerifyLogicalToPhysicalTileTable` (once per boot) after overworld BG init — **`AGB_ASSERT`** on:

- Each entry uses **only** the **10-bit** TID field (`value == (value & 0x3FF)`).
- Each physical TID **≤ `MAP_BG_FIELD_TILESET_MAX_TILE_INDEX`** (fits standard tilemap encoding + current **`MAP_BG_*`** capacity).
- **Door scratch contract:** indices **`MAP_BG_FIELD_DOOR_TILE_RESERVED_FIRST` … `MAP_BG_FIELD_TILESET_MAX_TILE_INDEX`** must be **identity** (`entry[i] == i`) while **`field_door.c`** uploads anim tiles to **`DOOR_TILE_START`** and **`BuildDoorTiles`** feeds those indices through **`DrawDoorMetatileAt` → `FieldMapTranslateMetatileTileHalfwordForFieldBg`**.

**Offline:** `tools/validate_field_map_logical_to_physical_inc.py` (or **`tools/check_direction_b_offline.py`**) on **`src/data/field_map_logical_to_physical_identity.inc`**:

| Mode | Use |
|------|-----|
| default | range + 10-bit clean + door tail; **warn** on **collision** (multiple logical → same physical) |
| **`--strict-identity`** | shipped vanilla identity table |
| **`--strict-injective`** | CI / hacks: **error** if any physical TID is shared (no silent upload overwrites) |
| **`--no-collision-warnings`** | quiet stderr; still fails on **`--strict-injective`** |
| **`--hint-primary-secondary`** | non-fatal notes when primary-semantic logical (**0..639**) maps to physical **≥640** or secondary (**640..tail-1**) maps to **&lt;640** |
| **`--no-door-contract`** | only when door upload + table policy are redesigned **together** |

**Make:** `make check-field-map-logical-physical-table` runs the default validator on **`src/data/field_map_logical_to_physical_identity.inc`**.

**CI / local (canonical portable gate):** **`.github/workflows/portable_host.yml`** runs the **same default** validator before the SDL build; **`tools/verify_portable_default.sh`** and **`tools/verify_portable_default.ps1`** do the same so local runs match the merge gate. **`--strict-injective`** is **not** part of that default path — add a **separate** CI job or manual step for hacks that require injective tables.

**Runtime (debug):** first non-identity table triggers a **one-shot `AGB_WARNING`** if **any** physical TID collision exists (mirrors validator intent; use offline **`--strict-injective`** for hard gates).

**Not asserted (document / future work):** **Surjectivity** / “every physical slot filled”; **renderer** sampling if tilemaps ever bypass the writer choke point.

**Compressed remapped upload:** non-identity **compressed** tilesets use **per-tile `LoadBgTiles`** + **`WaitDma3Request`** per tile — higher cost than identity bulk/temp-buffer path; document for hacks profiling large maps.

**Upload (landed):** `CopyTilesetToVram` / `CopyTilesetToVramUsingHeap` in **`fieldmap.c`** / **`src_transformed/fieldmap.c`** — **identity** keeps one bulk `LoadBgTiles` / legacy decompress helpers; **non-identity** uploads each **combined logical** tile (primary offset **0**, secondary offset **`MAP_BG_SECONDARY_TILESET_CHAR_BASE_TILE`**) to **`gFieldMapLogicalToPhysicalTileId[logical]`**. Remapped **compressed** path uses **`MallocAndDecompress`** + per-tile `LoadBgTiles` + **`WaitDma3Request`** before **`Free`** (differs from identity temp-buffer lifecycle by design).

**Door + first remapping (codebase answer):** **`FieldMapVerifyLogicalToPhysicalTileTable`** and **`DOOR_TILE_START` → `MAP_BG_FIELD_DOOR_TILE_RESERVED_FIRST`** mean the **first** safe experiments should **keep the 8 tail logical indices identity** while remapping only below **`MAP_BG_FIELD_DOOR_TILE_RESERVED_FIRST`**, **unless** `field_door` upload / halfword construction are refactored in the same change.

**Non-identity hacks** are now **technically servable** (upload + tilemap translator agree); they remain **disciplined** by door-tail verification and the absence of automatic bijection checks.

3. **Defer A/C** unless the visual design **requires** multi-base **per scanline** or portable-only virtual VRAM — both are **program-sized** bets.

---

## 4. Direction D — closure (inventory + strict-cap tooling)

**Status:** **Closed** for the scoped charter (caps aligned with **`MAP_BG_*`**, offline fail-closed tooling, optional per-compiled-index role caps, one debug assertion on translated TIDs). Anything requiring **> 1024** tile **numbers**, non-contiguous char layout, or renderer/virtual planes is **out of scope** — see directions **A / B / C** in §2.

**Delivered:**

| Area | Artifact |
|------|-----------|
| Inventory | `docs/architecture/project_b_field_tilemap_tile_index_inventory.md` |
| Shared numbers | `tools/portable_generators/map_visual_policy_constants.py` |
| Compiled table roles | `tools/portable_generators/compiled_tileset_table_roles.py` |
| ROM validators | `validate_compiled_tileset_asset_profile_directory.py` (combined + **`--enforce-role-tile-caps`**); **`validate_compiled_tileset_tiles_directory.py`** (**`--enforce-role-tile-caps`**: strict LZ size == role cap; raw **`max-blob`** clamped); palette + metatile gfx/attr validators with MAP_BG defaults + explicit exceed flags |
| Map grid words | **`tools/validate_map_layout_block_bin.py`**, **`validate_map_layout_rom_packs.py --validate-block-words`** (Project C boundary; same policy constants) |
| Field TID table | **`tools/validate_field_map_logical_to_physical_inc.py`**, **`tools/check_direction_b_offline.py`** (imports policy constants) |
| Runtime (debug) | **`FieldMapTranslateMetatileTileHalfwordForFieldBg`**: **`AGB_ASSERT(phys <= MAP_BG_FIELD_TILESET_MAX_TILE_INDEX)`** |
| Offline smoke | **`tools/check_direction_d_offline.py`**; **`make check-direction-d`** (= **`check-validators`** + smoke); CI + **`verify_portable_default.*`** |

**Not attempted (explicitly non-goals for D):** per-pixel **field_camera** range checks beyond the translate assert; automatic ROM generation of **`tile_uncomp_bytes`** (validators only); surjective / filled physical slot proofs.

**Next program:** **Direction A / C** or **joint B + C** if map encoding changes; optional **larger Direction B** (renderer / ROM-versioned addressing) only with an explicit product decision (§2 direction **B**).

---

## 5. Direction B — closure (logical→physical TID indirection)

**Status:** **Closed** for the **bounded** charter: one **1024-entry** logical→physical table, **single writer** translation for metatile/door halfwords, **upload** scattered to **physical** slots per table, **debug** verification + **offline** validator + **smoke** script. **Non-identity** hacks are supported subject to **door-tail** identity and collision discipline (see §3).

**Delivered:**

| Area | Artifact |
|------|-----------|
| Contract type | **`struct FireredFieldBgTileAddressing`** + **`gFieldMapLogicalToPhysicalTileId`** (`fieldmap.h` / `src/field_map_tile_logical_to_physical.c`) |
| Translate | **`FieldMapPhysicalTileIndexFromLogical`**, **`FieldMapTranslateMetatileTileHalfwordForFieldBg`**, **`FieldMapFieldTileUploadUsesIdentityLayout`** |
| Writers | **`field_camera`** / **`src_transformed/field_camera.c`** `DrawMetatile`; door path via same translator |
| Upload | **`CopyTilesetToVram`** / **`CopyTilesetToVramUsingHeap`** (`fieldmap.c` / `src_transformed/fieldmap.c`) |
| Runtime checks | **`FieldMapVerifyLogicalToPhysicalTileTable`** (debug); collision **warning** when non-identity |
| Data | **`src/data/field_map_logical_to_physical_identity.inc`** (+ generator **`tools/generate_field_map_logical_to_physical_identity_inc.py`**) |
| Offline | **`tools/validate_field_map_logical_to_physical_inc.py`**; **`tools/check_direction_b_offline.py`**; **`make check-field-map-logical-physical-table`** / **`make check-direction-b-offline`** / **`make check-direction-b`**; folded into **`make check-validators`** |

**Not attempted (non-goals for bounded B):** surjective physical-slot proofs; renderer decode of **logical** TIDs; automatic codegen of non-identity tables from ROM profiles.

---

## 6. Relationship to Project C

**Metatile ID** semantics on the **map grid** (640 split, saved map format) remain **Project C**. This redesign focuses on **tile char addressing** for **graphics**; if **logical** metatile or **grid** width changes, **C + B** must be scheduled together.

---

## Related docs

- `docs/architecture/map_visual_three_frontiers.md` — Lane B history and **natural boundary** before this program.
- `docs/architecture/project_b_field_tilemap_tile_index_inventory.md` — **tilemap / tile-index** consumer map.
- `docs/portable_rom_map_layout_metatiles.md` — ROM pack seams and validators.
