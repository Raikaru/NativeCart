# Map / layout visual architecture — three frontiers

This document captures **parallel research** on the post-ROM-profile blockers for overworld map visuals. It ranks them for **program order** and records the **first bounded implementation** chosen in-repo.

## Lane A — Palette upload policy

**Problem:** `LoadMapTilesetPalettes` / `LoadTilesetPalette` fix **how many** palette halfwords are copied into `gPlttBufferUnfaded` and **which BG palette slots** they occupy. Secondary tilesets use bundle row **`NUM_PALS_IN_PRIMARY`** as the first row pointer (`FireredPortableCompiledTilesetPaletteRowPtr` / `tileset->palettes[…]`). ROM bundles may be **larger** than vanilla, but **DMA byte counts** and **slot bases** stay tied to **`NUM_PALS_IN_PRIMARY`** / **`NUM_PALS_TOTAL`**.

**Why the stack stops here:** Any change to row counts or slot placement affects **quest-log tint**, **ApplyGlobalTintToPaletteEntries**, and **every other screen** that assumes classic BG palette slot usage. Policy must be **named**, **versioned**, and **cross-checked** against non-field code paths.

**First step (landed):** Centralize **map BG palette policy** as explicit macros in `fieldmap.h` (`MAP_BG_*`) and route **`LoadPrimaryTilesetPalette` / `LoadSecondaryTilesetPalette`** and the secondary **bundle row index** in `LoadTilesetPalette` through those names (vanilla / non-PORTABLE).

**Second step (behavior-capable, PORTABLE):** Optional ROM word **`FIRERED_ROM_FIELD_MAP_PALETTE_POLICY_OFF`** selects **primary vs secondary row counts** (sum **`NUM_PALS_TOTAL`**). Runtime accessors **`FireredPortableFieldMapBgPalettePrimaryRows()` / `…SecondaryRows()`** drive **`LoadTilesetPalette`** sizes, **`BG_PLTT_ID`** secondary base, ROM bundle **secondary row** selection, **`FireredPortableCompiledTilesetPalettePtrForLoad`**, and **Teachy TV**’s primary/secondary split. **`FireredRomFieldMapPalettePolicyRomActive()`** is **TRUE** only when a **non-zero** valid wire word was applied.

**Third step (field-adjacent consumers):** **`LoadMapFromCameraTransition`** applies weather gamma to **secondary map slots** via **`FireredPortableFieldMapBgPalettePrimaryRows()` .. `NUM_PALS_TOTAL-1`** (not a fixed `7..12` loop). **Pokémon League lighting** (`field_specials.c`) loads/tints the **first secondary** slot and uses a matching **`BlendPalettes`** bit derived from the same accessor.

**Natural boundary (Project A “done enough”):** Row-split policy for **map tilesets** is wired for **upload**, **ROM bundles**, **Teachy TV**, **weather gamma on secondary rows**, and **League lighting**. Remaining palette work is mostly **not** “primary vs secondary rows within **`NUM_PALS_TOTAL`**” but **cross-subsystem layout**: which **non-map** screens own **BG slots 13–15**, how **OBJ** palette bands interact with field effects, and whether **`NUM_PALS_TOTAL`** itself should ever change (still **13** today). **Quest log** backup/restore uses spans that already cover **all map BG slots (0–12)** plus **slot 13** for playback UI (`CopyPaletteInvertedTint` count **`0xDF`**); that is **independent** of the primary/secondary **split** inside 0–12. **Optional fidelity** (e.g. renaming magic counts to `NUM_PALS_TOTAL` expressions) is hygiene, not a missing policy seam. Further unification belongs to a **broader palette-layout contract**, not another Project A pass.

**Size:** Bounded ROM pack + call-site wiring (no VRAM / metatile ID program).

**Likely write set:** `cores/firered/portable/firered_portable_rom_map_layout_metatiles.c`, `include/map_layout_metatiles_access.h`, `src_transformed/fieldmap.c`, `src_transformed/teachy_tv.c`, `docs/portable_rom_map_layout_metatiles.md`, this file.

**Overlap:** Touches **VRAM** only indirectly (palette RAM → tile rendering). Strong coupling to **metatile** only if palette rows and metatile palette indices are decoupled in hacks.

**Best first project?** **Yes for a first code step** — smallest blast radius if values stay vanilla; **ROM profile work already assumes** this layout.

---

## Lane B — VRAM / BG char layout policy

**Problem:** Map tilesets use **BG2**, `LoadBgTiles(2, …)` with **primary** at char index base **0** and **secondary** contiguous after **`NUM_TILES_IN_PRIMARY`** tiles. `LoadBgTiles` **size is `u16`** bytes for uncompressed paths. Larger **tile sheets** (profile-driven via **`FireredPortableCompiledTilesetVramTileBytesForLoad`**) can exceed **meaningful** char space or **hardware**-style limits on the portable renderer. **Metatile** tilemaps still index into the **combined** char block capped by **`NUM_TILES_TOTAL`** (Project C if that encoding changes).

**First bounded step (landed):** Named **compile-time** policy surface in **`fieldmap.h`**: **`MAP_BG_PRIMARY_TILESET_CHAR_BASE_TILE`**, **`MAP_BG_SECONDARY_TILESET_CHAR_BASE_TILE`**, **`MAP_BG_PRIMARY_TILESET_TILE_COUNT`**, **`MAP_BG_SECONDARY_TILESET_TILE_COUNT`**, **`MAP_BG_FIELD_TILE_CHAR_CAPACITY`**, and **`MAP_BG_FIELD_TILESET_GFX_BG`** (vanilla **BG2** — target of **`LoadBgTiles` / `Decompress*BgGfx*2`** for map tilesets). **`Copy*TilesetToVram*`** (`fieldmap.c`), **Teachy TV** map tile staging, and **door** scratch tiles (`DOOR_TILE_START` from **`MAP_BG_FIELD_TILE_CHAR_CAPACITY`**) route through these names. **Behavior unchanged** (same numeric layout as before); future **runtime** bases/capacity or **multi-BG** work has a single **documentation + edit** anchor (parallel to Project A’s first **`MAP_BG_*` palette** step).

**Second bounded step (landed, PORTABLE):** **`CopyTilesetToVram` / `CopyTilesetToVramUsingHeap`** validate **`FireredPortableCompiledTilesetVramTileBytesForLoad`** against **`MAP_BG_FIELD_TILE_CHAR_CAPACITY`** (no **offset+tileCount** overflow past combined char space), **32-byte** tile alignment, and (uncompressed **`LoadBgTiles`**) **`u16`** byte limit **`≤ 0xFFFF`**. **Debug** builds **`AGB_ASSERT`**, **NDEBUG** skips the upload on violation (avoids silent **`(u16)`** truncation / VRAM stomp).

**Third bounded step (landed):** **`MAP_BG_FIELD_TILESET_CHAR_BASE_BLOCK`** documents **`BgTemplate.charBaseIndex`** for the field tileset BG. **`overworld.c`** uses **`STATIC_ASSERT(NELEMS(sOverworldBgTemplates) > MAP_BG_FIELD_TILESET_GFX_BG)`** and, on **PORTABLE**, **`FieldOverworldVerifyFieldTilesetBgAgreement()`** after **`InitBgsFromTemplates`** ( **`sOverworldBgTemplates[bg].bg`**, **`GetBgAttribute` → `charBaseIndex` / `baseTile`**, **4bpp `paletteMode`**) so named policy matches **live** BG config. **`field_camera.c`** comment ties metatile tilemap indices to **`MAP_BG_FIELD_TILESET_GFX_BG`**.

**Fourth bounded step (landed):** **`MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES`** names the **byte** span of the field map tileset char block (**`MAP_BG_FIELD_TILE_CHAR_CAPACITY` × 32**) with **`STATIC_ASSERT` ≤ `0x18000`**. **`fieldmap.c`** uses that macro for the upload end-address check. **PORTABLE `dma3_manager.c`** **`PortableDmaDestBufferRangeOk`** rejects **`RequestDma3Copy` / `RequestDma3Fill`** (and drops bad queued ops) that would extend past emulated **VRAM (`0x18000`) / PLTT / OAM** — the **backing guarantee** behind **`LoadBgVram` → `RequestDma3Copy`**.

**Fifth bounded step (landed — char-block / tile-index contract):** **`fieldmap.h`** documents **linear** addressing `charBase * BG_CHAR_SIZE + tileIndex * 32` (same as **`engine_render_4bpp_tile`**), **`MAP_BG_FIELD_TILESET_MAX_TILE_INDEX`**, and **`STATIC_ASSERT`s**: capacity **≤ 1024** (standard **10-bit** tilemap tile number space) and **`MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES` ≤ 2 × `BG_CHAR_SIZE`** (vanilla field tileset spans **two** hardware **16KiB** char slabs from one base). Comments in **`field_camera.c`** / **`engine_renderer_backend.c`** cross-link.

**Natural architectural boundary (Project B, after step 5):** Anything that needs **> 1024** field tile IDs, **non-contiguous** char layout, **multiple char bases** for one logical map tileset, or **wide** tilemap entries is **no longer** a small contract patch — it implies **tilemap format**, **`field_camera` / metatile** encoding, **upload** path, and **renderer** sampling together. Treat as **layout redesign**, not another incremental **`MAP_BG_*`** step.

**Larger redesign program (chartered):** `docs/architecture/project_b_field_tileset_redesign.md` — ranks **multi-char-base**, **logical indirection**, **renderer virtual plane**, and **strict-capped** paths; lists **coupling points**, **tooling**, and a **phased** implementation order. **Tilemap/tile-index inventory:** `docs/architecture/project_b_field_tilemap_tile_index_inventory.md`. **Direction D (closed in-tree):** asset-profile + tile-blob + metatile validators aligned to **`MAP_BG_*`**, optional **`--enforce-role-tile-caps`**, offline **`tools/check_direction_d_offline.py`** / **`make check-direction-d`**, debug assert on translated TIDs in **`FieldMapTranslateMetatileTileHalfwordForFieldBg`** (see redesign doc §4). **Direction B (bounded charter closed):** same translator + table + upload pairing + offline gates as **`project_b_field_tileset_redesign.md`** §5 — including **`struct FireredFieldBgTileAddressing`**, **`tools/check_direction_b_offline.py`** / **`make check-direction-b-offline`**, and **`validate_field_map_logical_to_physical_inc.py`** (default collision **warnings**; **`--strict-injective`** opt-in for hacks). **Upload:** **`Copy*TilesetToVram*`** follows the table when not identity. **Door tail** stays identity until **`field_door`** is refactored with the table. **Deferred:** renderer second indirection / ROM-versioned addressing bundle (larger B).

**Overlap:** **Palette** must stay consistent with **which tiles** use which **palette indices**. **Metatile** graphics reference **tile indices** into the combined char block.

---

## Lane C — Metatile ID / map-format semantics

**Problem:** Map grid values use a **fixed split**: ids **&lt; 640** → primary metatile graphics/attributes, **640..1023** → secondary. Runtime uses **`MAP_GRID_METATILE_*`** helpers from **`fieldmap.h`** (**`field_camera`**, **`fieldmap`** attributes, **shop**, **Teachy TV**, portable metatile ROM core); **`NUM_METATILES_*`** remain the underlying numeric definitions. ROM tables may be **physically larger** (profile-driven spans), but **encoding** of ids in **map data** is still vanilla.

**Why the stack stops here (for a full redesign):** Changing **encoding** requires **map format**, **tools**, **save compatibility**, and **every consumer** of metatile ids — a **multi-subsystem** project.

**First bounded step (landed — Project C start):** Named **`MAP_GRID_METATILE_*`** policy + **`MapGridMetatileIdUsesPrimaryTileset` / `…SecondaryTileset` / `MapGridMetatileSecondaryRowIndex`** in **`fieldmap.h`**, with **`GetAttributeByMetatileIdAndMapLayout`** and **`DrawMetatileAt`** (**`src/`** + **`src_transformed/`**) routed through the helpers. **Vanilla behavior preserved**; **`NUM_METATILES_*`** remain the numeric source of truth. Charter + inventory: **`docs/architecture/project_c_metatile_id_map_format.md`**.

**Next boundary (Project C — block words):** **Encoded-space** policy is landed (**`MapGridMetatileIdIsInEncodedSpace`**). The **large** block-word program (**`map_grid_block_word.h`**, save **`mapView`** hook, **`mapjson`** + validators, **`make check-map-layout-block-bins`**) is **landed** — see **`docs/architecture/project_c_map_block_word_and_tooling_boundary.md`**.

**Next architecture program (after that C slice):** **Direction D** strict-cap tooling is **closed** (§4); **bounded Direction B** is **closed** (§5). Follow-on is **larger Project B** (A/C-scale char layout, optional renderer-indirection) or **joint B+C** layout decisions. Lane C **grid encoding** changes beyond vanilla **`u16`** words remain a **joint B+C** decision.

**Overlap:** **VRAM** tile index ranges must match **metatile → tile** expansion. **Palette** row choice in metatile tiles still assumes **valid** palette indices.

**Best first project?** **Yes for a bounded first code step** — same pattern as Projects A/B (**named policy + thin central wiring**). A **full** Lane C encoding change remains a **larger** program after that hook.

---

## Rank (for first architecture move)

| Project              | Leverage | Bounded 1st step | Clarity | Risk | Overlap |
|---------------------|----------|------------------|---------|------|---------|
| A Palette policy    | High     | **Good** (macros)| High    | Low  | Medium  |
| B VRAM / BG layout  | High     | Poor (design)    | Medium  | High | High    |
| C Metatile / map ID | High     | **Good** (policy + helpers + doc; central sites) | Medium | Med  | High    |

**Conclusion:** **Lanes A, B, and C** each have a **first bounded code step** under the current architecture: **`MAP_BG_*`** (**palette** then **char/tile layout**), then **`MAP_GRID_METATILE_*`** (**metatile id interpretation**). **Full** VRAM/renderer redesign and **non-vanilla map encoding** remain **larger** programs beyond those hooks.

## Related docs

- `docs/architecture/README.md` — index of map-visual architecture notes (Projects B/C).
- `docs/portable_rom_map_layout_metatiles.md` — ROM seams and **architectural boundary** for profile-sized metadata.
- `docs/architecture/project_b_field_tileset_redesign.md` — **larger Project B** field tileset redesign (post–small-B charter).
- `docs/architecture/project_c_metatile_id_map_format.md` — **Project C** metatile ID / map-format charter.
- `docs/architecture/project_c_map_block_word_and_tooling_boundary.md` — Project C **block word** program (**`u16`** layout, save **`mapView`**, tooling) — infrastructure landed; joint changes with **Project B** if tile id space changes.
- `include/fieldmap.h` — **`MAP_BG_*`** field map **palette** and **char / tile count** policy hooks; **`MAP_GRID_METATILE_*`** metatile ID policy.
