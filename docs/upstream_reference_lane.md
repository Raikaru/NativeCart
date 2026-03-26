# Upstream reference lane (vanilla FireRed vs this fork)

This document defines **policy and reproducibility** for a **vanilla / pret reference lane**: what to match, what is fork-intentional, how to interpret **`make compare_*`** and ROM diffs, and **what is not claimed** about this repository’s compare status.

**Doc roles (read this first):**

| Document | Responsibility |
|----------|----------------|
| **`upstream_reference_lane.md` (this file)** | Lane definition: **§ Lane state** (stopping point, metrics), drift categories, **pinned pret baseline (§0)**, **observed compare/ROM facts** (when recorded), diagnosis workflow. **Does not** duplicate step-by-step merge commands. |
| **`upstream_pokefirered.md`** | **Practical how-to:** add `pret` remote, merge/regenerate **jsonproc** outputs, **Makefile** include path, fork-specific **agbcc** / daycare notes. |

Top-level **README** / **INSTALL** point here for orientation; they **do not** imply **`make compare_firered` is green** on fork `main`.

---

## Lane state: bounded pret-shaped reconciliation closed

**Stopping point:** The **active queue** of **source-level, pret-shaped, bounded `.text` wins** in shared vanilla **`src/*.c`** (accidental drift vs **pret**) is **treated complete** as of **2026-03**. What remains vs the **§0** pin is **intentional fork behavior**, **Project C / Direction B** structure, **toolchain/link-layout**, or **explicitly deferred** items — **not** a standing to-do list for casual byte-shaving.

**Further `make compare_*` / pret parity work** requires a **new explicit charter**: stated **goal** (bit-identical SHA vs diagnostic layout), **metric** (see **Metrics policy** below), **hypothesis** (which TU / which section), and **non-goals** (e.g. will not strip **Project C**). **Do not** extend the old queue by map-diff mining alone.

| Bucket | What it is |
|--------|------------|
| **Landed bounded wins** | Pret-aligned changes shipped with **object- or ROM-level** verification: **`window.o`**, **`pokemon.o`**, **`field_specials.o`** (**`map_event_ids.h`** / **`MoveDeoxysObject`**), **`new_game.o`**, **`battle_ai_script_commands.o`**, **`event_object_movement.o`**; plus **vanilla** omission of **`FieldMapVerifyLogicalToPhysicalTileTable`** body (**`PORTABLE && !NDEBUG`** only) — real **ROM / late-bank** footprint effect. |
| **Accepted intentional fork cost** | Shared **`src/*.o`** positives **audited and retained**: **`overworld.o`** (**`BindMapLayoutFromSave`**); **`save.o`** (PKHeX fingerprint + flash path); **`battle_setup.o`** (**`TRAINER_PARAM_LOAD_PTR`**); **`shop.o`** / **`teachy_tv.o`** (**Project C** metatile path). **Closed** — not default reconciliation targets. |
| **Structural architecture (≠ pret drift)** | **`field_camera.o`**, **`fieldmap.o`** — **Project C** block words, **`FieldMapTranslateMetatileTileHalfwordForFieldBg`**, layout accessors. **Not** “accidental pret gap”; reconciling to pret would **reject** documented map policy. |
| **Deferred toolchain / link-layout** | **`.text.__stub`**, **`extra`** **`.text`** placement vs **`ld_script.ld`**, headline **`.text`** vs total code size — understood; changes only under an explicit **policy/metric** decision. |

---

## Success criteria (this lane, current)

1. **Reproducible vanilla reference:** **`make firered` `MODERN=0`** (or documented CI) produces a binary that can be **measured**; **§0.1** records snapshots when someone re-verifies.  
2. **`make compare_*` remains a diagnostic / reference tool** for this fork’s **`main`**, **not** an assumed green gate unless the project explicitly adopts SHA parity as a requirement.  
3. **Drift is classifiable** (**§3**, **§0.3** history): intentional fork, structural architecture, generated data, build env, or toolchain — without reopening **closed** audits without a **new hypothesis**.  
4. **No implied obligation** to shrink the pret **`.text`** gap by default.

---

## Metrics policy

- **Headline map `^.text` at `0x08000000`:** size of the **first** **`.text`** output section from **`ld_script.ld`**. It **does not** include **`extra`** / late-linked **`src/*.o(.text)`**; it can stay **unchanged** when code shrinks **only** in the late bank.  
- **Use for “did we shrink code?”:** **`ld --print-memory-usage`** **ROM Used Size**; per-object **`.text`** lines in **`pokefirered.map`**; **`arm-none-eabi-size`** on **`*.o`** / **`.elf`** when needed.  
- **Footprint wins count** when **ROM** or the relevant **`.o`** shrinks **even if** headline **`.text`** does not move (padding, placement, stub removal).  
- **Out of scope for pret-bytes-only passes:** patches that trade **Project C** correctness, **PORTABLE** semantics, save fingerprint behavior, or trainer **ROM-pointer** loading for **pret** object sizes — unless **architecture** explicitly approves the trade.

---

**Operational status:** The lane is **documented**, not **certified**. Bit-identical ROM parity is **not** claimed unless someone runs **`make compare_firered`** successfully and records it in **§0.1**. **Bounded pret-shaped source reconciliation is closed** — see **Lane state** above.

---

## 0. Pret source baseline (**pinned**)

Upstream comparison uses **`https://github.com/pret/pokefirered`** (not vendored in-tree). **Canonical lane pin** (recorded when this section was last set):

| Field | Value |
|-------|--------|
| **Remote name** | **`pret`** → `https://github.com/pret/pokefirered.git` |
| **Ref used** | **`pret/master`** (upstream default branch) |
| **Commit** | **`9bbcab4b06ba87c3de796325461cb45a8803b9d9`** |
| **Commit date (author, ISO)** | **`2026-03-23T08:55:42-04:00`** (`git log -1 --format=%aI pret/master`) |
| **Subject** | `Remove modern compiler warning by removing fakematches (#742)` |

**Reproduce locally:** `git remote add pret https://github.com/pret/pokefirered.git` (if missing), then **`git fetch pret master`** and verify with **`git rev-parse pret/master`**. For a **byte-for-byte** match to this pin, diff or checkout **`9bbcab4b06ba87c3de796325461cb45a8803b9d9`** — upstream **`master` moves**; refresh this table when you intentionally advance the lane.

**Fetch note:** A shallow **`git fetch pret master --depth=1`** is enough to resolve **`pret/master`** to the tip at fetch time; a **full** fetch is fine if you need older pret history.

---

## 0.1 Observed status (fork snapshot — update when re-verified)

Recorded so the lane is **grounded in one real outcome**, not implied greenness.

| Field | Value |
|-------|--------|
| **Last lane verification (UTC)** | **`2026-03-25T12:00:00Z`** (approx.; MSYS2 **`make firered MODERN=0`** after **`fieldmap.h`** + **`field_map_tile_logical_to_physical.c`**: verifier body only under **`PORTABLE && !NDEBUG`**) |
| **Pinned pret at verification** | **`pret/master`** **`9bbcab4b06ba87c3de796325461cb45a8803b9d9`** — map diff uses pret **`pokefirered.map`** from that pin (unchanged). |
| **Repository `HEAD` (snapshot)** | `905a0931f1eff736240c78a8c17df0759a4b24a6` (`905a0931`) (working tree may include uncommitted lane edits) |
| **`make compare_firered`** | **MSYS2** bash, **`PATH`** includes **`/mingw64/bin`**, **`mingw32-make compare_firered GAME_VERSION=FIRERED MODERN=0`**. **Link / `gbafix` / `objcopy` succeed**; **`rom`** / SHA step **still fails** (**`pokefirered.gba: FAILED`**). **Not green.** |
| **Expected SHA1 (`firered.sha1`)** | `41cb23d8dccc8ebd7c649cd8fbb58eeace6e2fdc` |
| **Observed SHA1 (built `pokefirered.gba`, same run)** | `e67bf89e8f766730eb7f7e6fb74aa6e4b3167130` |
| **First ROM byte diff (`baserom.gba` vs `pokefirered.gba`)** | **Not re-measured** (no **`baserom.gba`** in this workspace). **Story unchanged in kind:** header word at file **`0x128`** tracks **`gMonFrontPicTable`** ROM placement; that table’s VMA moved **`−0x18`** toward retail after this **`overworld`** shrink (see **§0.2**). |
| **Differing byte count (same run, full image)** | **Not re-measured** here. |

**Blocker (if treating compare green as the bar):** **`make compare_firered`** stops at **ROM SHA1 check**; parity requires reconciling fork layout/content with retail + pret, not just refreshing this doc.

Re-run **`make compare_firered`** and re-check **first offset** / **differing-byte count** after meaningful changes; refresh this table **or** add a dated subsection if you keep historical snapshots.

---

## 0.2 First-divergence diagnosis (`gMonFrontPicTable` pointer @ `0x08000128`)

**Cause category (reconciliation pass, pinned pret `9bbcab4b…`):** **Link-time ROM layout drift** — not a mistaken field in **`sGFRomHeader`**, and not (at the table head) divergent **`gMonFrontPicTable`** payload.

| Check | Result |
|-------|--------|
| **Pointer in ROM** | Retail **`baserom.gba`**: word @ file **`0x128`** → ROM **`0x08000128`** = **`0x082350ac`**. Fork build (**`pokefirered.map`**, post-**`overworld`** **`PORTABLE`**-only **`FieldMapVerifyLogicalToPhysicalTileTable`**): **`gMonFrontPicTable`** @ **`0x08235158`**. Delta **+`0xac` (172)** vs retail word. |
| **`gMonFrontPicCoords` payload** | **`0x6e0`** bytes at retail coords base vs fork coords base are **byte-identical** when aligned at each build’s coords base (fork coords sit **`0xac`** later than retail’s at this snapshot). |
| **`gMonFrontPicTable` payload (start)** | First **4096** bytes at each ROM’s table base **match** retail vs fork — first **content** divergence in that neighborhood appears **later** (~**`0x3DE0`** into the table region), not at the header pointer. |
| **`src/rom_header_gf.c` vs pret** | Same **`monFrontPics = gMonFrontPicTable`** initializer; fork-only **`PORTABLE`** branches do not apply to vanilla **`MODERN=0`** compare for that field. |
| **`src/data.c` vs pret (git)** | Only **`#include "constants/species.h"`** and **`STATIC_ASSERT`**s **after** the pokémon graphics **`#include`s** — **`STATIC_ASSERT`** is typedef-based and does **not** explain **300** bytes of **`.rodata`**. |
| **Other drift vs pret** | Same **`data.o`** layout story as **§0.3**. Fork **`.text`** total is **`0x15fa60`** vs pret **`0x15f9b4`** → **`+0xac`** after **`overworld.o`** **`FieldMapVerifyLogicalToPhysicalTileTable`** **`PORTABLE`**-only (see **§0.3**). **`src/window.o(.text)`** matches pret (**`0xf40`**); **`src/pokemon.o(.text)`** matches pret (**`0x74b4`**); **`src/field_specials.o(.text)`** matches pret (**`0x2c10`**); **`src/new_game.o(.text)`** matches pret (**`0x2cc`**); **`src/battle_ai_script_commands.o(.text)`** matches pret (**`0x2a9c`**); **`src/event_object_movement.o(.text)`** matches pret (**`0xb51c`**) (see **§0.3**). |

**Bounded step (earlier pass):** **No source patch landed** — see **§0.3** for **map / `data.o`** follow-up.

---

## 0.3 Map reconciliation pass (pinned pret `9bbcab4b…`, `.text` gap vs pret)

**Pret `pokefirered.map` at the pin:** **Available** (e.g. detached worktree build). Compare to fork **`pokefirered.map`** from the same **`GAME_VERSION=FIRERED`**, **`MODERN=0`** link.

**Resolved: the `0x12c` is not `src/data.o` layout drift vs pret**

| Check | Result |
|-------|--------|
| **`src/data.o(.rodata)` size** | **Same** on fork and pret: **`0x13c03`**. |
| **Offsets inside `data.o` `.rodata`** | **`gMonFrontPicCoords`** / **`gMonFrontPicTable`** sit at the **same** offsets from the **`data.o` `.rodata` base** on both maps — internal monster-table layout **matches pret**. |
| **Aggregate `.text`** | Pret **`.text`** total **`0x15f9b4`**; fork **`0x15fa60`** (**measured**, MSYS2 **`make firered`**, **`GAME_VERSION=FIRERED`**, **`MODERN=0`**). Difference **`+0xac`** on the fork vs pret. |

**Measured `window.o` impact (same builds)**

| Artifact | `src/window.o` **`.text`** | Notes |
|----------|---------------------------|--------|
| **Pret map (pin)** | **`0xf40`** (3904) | Reference. |
| **Fork map (before this reconciliation)** | **`0xf58`** (3928) | **`+0x18`** vs pret — redundant **`GetWindowTileDataAddress`** body on vanilla. |
| **Fork object (`arm-none-eabi-size -A build/firered/src/window.o`)** | **`0xf40`** (3904) | Matches pret **`.text`** size. |
| **Fork map (after rebuild)** | **`0xf40`** at **`0x08003b20`** | **No** fork-vs-pret **`window.o`** delta. |

**Total `.text` shrink vs the prior fork map:** section total dropped by **`0x10`** (**`0x15fae0` → `0x15fad0`**), not **`0x18`**: **`window.o`** lost **`0x18`**, but **link-time padding / placement** elsewhere in **`.text`** absorbed **`0x8`** so the **observed** section delta is smaller than the **`window.o`** delta alone.

**Historical — shared fork-vs-pret **`src/*.o` `.text`** positives (pin map diff, fork larger); queue closed** — retained for diagnosis only; see **Lane state**

| `src/*.o` | Fork `.text` | Pret `.text` | Δ | VMA (fork) | Notes |
|-----------|-------------|--------------|---|------------|--------|
| **`daycare_egg_hatch.o`** | **`0xc3c`** | *(absent)* | **`+0xc3c`** | **`0x08046f20`** | **Fork TU split** — not a pret-shaped slice. |
| **`field_camera.o`** | **`0xf3c`** | **`0x96c`** | **`+0x5d0`** | **`0x0805a934`** | **Audited (map-queue close-out):** **`DrawMetatileAt` / `DrawMetatile`** use **`MapGridMetatileId*`** + **`FieldMapTranslateMetatileTileHalfwordForFieldBg`** (**Project C / Direction B**) — **not** pret-shaped drift. |
| **`daycare_egg_moves.o`** | **`0x348`** | *(absent)* | **`+0x348`** | **`0x08046b18`** | **Fork TU split.** |
| **`fieldmap.o`** | **`0x1450`** | **`0x112c`** | **`+0x324`** | **`0x08058a2c`** | **Audited (map-queue close-out):** **`MapGridBlockWordFromWireV1`** on init/connection fill, **`MAP_LAYOUT_METATILE_*`**, **`NormalizeSavedMapViewBlockWords`** — **Project C** block-word pipeline; **not** a bounded pret target. |
| **`field_map_tile_logical_to_physical.o`** | **`0x5c`** | *(absent)* | **`+0x5c`** | **`0x08eb0b20`** | **Late / link‑adjacent**; fork-only table TU. **`FieldMapVerifyLogicalToPhysicalTileTable`** body **omitted** on vanilla (**`PORTABLE && !NDEBUG`** only); **`FieldMapFieldTileUploadUsesIdentityLayout`** remains. |
| **`daycare_egg_species.o`** | **`0xc0`** | *(absent)* | **`+0xc0`** | **`0x08046e60`** | **Fork TU split.** |
| **`save.o`** | **`0x111c`** | **`0x10d4`** | **`+0x48`** | **`0x080d9ec8`** | **`EnsureSaveBlock2ExternalToolFingerprint`** + load/save hooks; **`PORTABLE`** **`STATIC_ASSERT`** / **`PortableFlash_PatchPkhexFrLgFingerprintAllSlots`**; pret **`REVISION >= 0xA`** **`svc_*`** paths removed on fork — **audited this pass**; **not** a bounded pret target. |
| **`teachy_tv.o`** | **`0x1720`** | **`0x16e8`** | **`+0x38`** | **`0x0815ac28`** | **`MAP_LAYOUT_METATILE_*` / `PORTABLE` Route1 block** + **Project C** **`MapGridMetatile*`** path (**`include/fieldmap.h`** cites **Teachy TV**); **audited this pass** — **not** a safe bounded pret target. |
| **`.text.__stub`** | **`0x28`** | **`0x0`** | **`+0x28`** | **`0x0815fa80`** | **Toolchain / linker stubs** — not a source-first target. |
| **`shop.o`** | **`0x174c`** | **`0x1728`** | **`+0x24`** | **`0x0809b23c`** | **`MapGridMetatileId*`** helpers — **intentional** fork. |
| **`battle_setup.o`** | **`0x1148`** | **`0x1128`** | **`+0x20`** | **`0x0807fda4`** | **`TRAINER_PARAM_LOAD_PTR`** — **intentional** fork (script / pointer path). |
| **`overworld.o`** | **`0x3e0c`** | **`0x3dfc`** | **`+0x10`** | **`0x08054c18`** | **`BindMapLayoutFromSave`** fork-only cost remains; **`FieldMapVerifyLogicalToPhysicalTileTable`** call sites under **`#ifdef PORTABLE`**; verifier **`.c`** body **gated** **`PORTABLE && !NDEBUG`** so vanilla link drops the symbol (**`arm-none-eabi-nm`** clean). Naive pret **`BindMapLayoutFromSave`** removal still **invalid** (historical note below). |
| **`event_object_movement.o`** | **`0xb51c`** | **`0xb51c`** | **`0`** | **`0x0805e788`** | **Matched pret (this pass):** pret **`for`** loop + second **`for`** in **`GetAvailableObjectEventId`** (semantically equivalent to the fork’s **`break`/`do`‑`while`** form); **`GetObjectEventScriptPointerByLocalIdAndMap`** uses pret’s direct **`->script`** on vanilla, **`#ifdef PORTABLE`** keeps **`NULL`** guard + **`OBJ_EVENT_TEMPLATE_SCRIPT_DEREF`**. **`arm-none-eabi-size`**: **`.text`** **`46364` (`0xb51c`)**. |

**Measured `pokemon.o` impact:** **`CreateBoxMon`** used a stack **`expVal`** + block even though **`ExperienceTableGet`** is already a **macro** to **`gExperienceTables`** when **PORTABLE** is unset — extra code vs pret’s **`&gExperienceTables[...][level]`**. **Fix:** pret-style **`SetBoxMonData`** for **`MON_DATA_EXP`** under **`#else`**, **PORTABLE-only** block unchanged. **`arm-none-eabi-size`**: **`pokemon.o` `.text`** **29876 (`0x74b4`)**, matching pret; total **`.text`** dropped **`0x10`** (**`0x15fad0` → `0x15fac0`**).

**Measured `field_specials.o` impact (this pass):** Fork had a **stale `include/constants/map_event_ids.h`** shaped like a **flat global LOCALID table** (comment claimed **`generate_localid_constants.py`**), so **`LOCALID_BIRTH_ISLAND_EXTERIOR_ROCK`** / **`LOCALID_BIRTH_ISLAND_DEOXYS`** were **`5` / `4`** instead of pret’s **per-map `mapjson` values `1` / `2`**. That inflated **`MoveDeoxysObject`** by **`0x4`** vs pret (**`arm-none-eabi-objdump -t`**: fork **`0xb4`**, pret **`0xb0`**). **Fix:** regenerate **`map_event_ids.h`** with **`tools/mapjson/mapjson event_constants firered <all data/maps/*/map.json> include/constants/map_event_ids.h`** (same rule as **`map_data_rules.mk`**). **`field_specials.o(.text)`** is now **`0x2c10`**, matching pret’s map. **Aggregate `.text`** total stayed **`0x15fac0`** on this link (**`pokefirered.map`**, **`arm-none-eabi-size -A pokefirered.elf`**) — the **`0x4`** came back elsewhere in the link (padding / other TU codegen), so **object-level** parity on **`field_specials.o`** did not move the **section** total.

**Vanilla codegen hygiene (same pass, `field_specials.c` + `src_transformed/field_specials.c`):** QuestLog departed-map **`#else`** uses pret’s direct **`Overworld_GetMapHeaderByGroupAndId(...)->regionMapSectionId`**; **`#ifdef PORTABLE`** keeps scalar helper **`FireredRomMapHeaderScalarsRegionMapSec`**. League lighting uses **`FS_FIELD_BG_PAL_PRIMARY_ROW` / `FS_LEAGUE_LIGHT_BLEND_PAL_MASK`** (**`7` / `0x80`** when not **PORTABLE**) so vanilla matches pret’s literal immediates. **`map_header_scalars_access.h`** / **`map_layout_metatiles_access.h`** included only under **PORTABLE**.

**`new_game.o` / save fingerprint audit (landed):** Fork had extra **`gSaveBlock2Ptr->unkFlag1` / `unkFlag2`** writes in **`Sav2_ClearSetDefault()`** (comment: PKHeX vs RS detection on **RAM** right after **`ClearSav2()`**). **Save audit conclusion:** fields are **never read in-game**; **`EnsureSaveBlock2ExternalToolFingerprint()`** in **`src/save.c`** already applies the same pattern **before writes** and **after successful `LoadGameSave`**; **`NewGameInitData()`** still sets the flags as on pret. The **`Sav2_ClearSetDefault`** writes were **redundant** for gameplay and normal **`.sav`** / PKHeX workflows. **Reconciliation:** remove those writes (**`src/new_game.c`** + **`src_transformed/new_game.c`**); align **`save.c`** comment with real behavior. **Measured:** **`new_game.o(.text)`** **`0x2cc`**, pret match; aggregate **`.text`** **`0x15fac0` → `0x15faa8`** (**`−0x18`**); **`gMonFrontPicTable`** **`0x082351b8` → `0x082351a0`**; **ROM SHA1** **`38ea9cfa7d8f342db98c36b597ed107b114da132`** (**`make compare_firered`** still **not** green).

**Measured `battle_ai_script_commands.o` impact (this pass):** Fork ran an **out-of-range AI command index** guard (**`if (*sAIScriptPtr >= …)`**) on **vanilla** as well as **`PORTABLE`**, adding **`+0x28`** vs pret. **`git diff pret/master`** shows **`PORTABLE`** ROM script pointers + **`stdio`** trace already **`#ifdef PORTABLE`**; the guard is only needed when **`sAIScriptPtr`** can point at **ROM-backed** bytes. **Fix:** wrap the guard in **`#ifdef PORTABLE`** (use **`sizeof(sBattleAICmdTable) / sizeof(sBattleAICmdTable[0])`** inline; drop file-scope **`BATTLE_AI_CMD_COUNT`**). **`battle_ai_script_commands.o(.text)`** **`0x2a9c`**, pret match; aggregate **`.text`** **`0x15faa8` → `0x15fa80`** (**`−0x28`**); **`gMonFrontPicTable`** **`0x082351a0` → `0x08235178`**; **ROM SHA1** **`908e63ac955e07383c836daa30f16c462a5904f9`**.

**Measured `event_object_movement.o` impact (this pass):** Residual **`+0x8`** vs pret was tied to **`GetAvailableObjectEventId`** (pret **`for`**/`for` codegen vs fork **`break`/`do`‑`while`**) and to a **vanilla** **`NULL`** check + redundant **`#else`** branch in **`GetObjectEventScriptPointerByLocalIdAndMap`**. **Equivalence proof (slot scan):** both forms advance **`i`** across the **active prefix**, stop at the **first inactive** index or **`OBJECT_EVENTS_COUNT`**, then scan **`[i, OBJECT_EVENTS_COUNT)`** for an **active** duplicate **(localId, mapNum, mapGroup)**. **Fix:** restore pret’s **`GetAvailableObjectEventId`**; **`#ifdef PORTABLE`** only: **`NULL`** guard + **`OBJ_EVENT_TEMPLATE_SCRIPT_DEREF`**; **`#else`:** pret **`GetObjectEventTemplateByLocalIdAndMap(...)->script`**. **`event_object_movement.o(.text)`** **`0xb51c`**, pret match; aggregate **`.text`** **`0x15fa80` → `0x15fa78`** (**`−0x8`**); **`gMonFrontPicTable`** **`0x08235178` → `0x08235170`**; **ROM SHA1** **`e5d8dededcdc68bdf9efe2656382e63a44d0dbfc`**. (**`src/event_object_movement.c`**, **`src_transformed/event_object_movement.c`**.)

**Earlier hygiene (same TU):** **`SetObjectSubpriorityByElevation`** was already matched to pret’s **`u16 y`** form; **agbcc** did not change **`.text`** size until this pass addressed the two hunks above.

**Audit-only pass (`battle_setup.o` / `shop.o`, pinned pret map):** **`battle_setup.o` `.text`** fork **`0x1148`** vs pret **`0x1128`** (**`+0x20`**); **`shop.o` `.text`** fork **`0x174c`** vs pret **`0x1728`** (**`+0x24`**). **`git diff pret/master -- src/battle_setup.c`**: adds **`TRAINER_PARAM_LOAD_PTR`** and switches trainer speech / return-address parameters from **`TRAINER_PARAM_LOAD_VAL_32BIT`** (**`SetU32`**) to **`TRAINER_PARAM_LOAD_PTR`** (**`SetPtr(specs->varPtr, T1_READ_PTR(data))`**) in **`TrainerBattleLoadArgs`** — **intentional** fork behavior (script words as **ROM bus / token** values decoded to **host `const u8 *`**, not accidental pret drift). **`git diff pret/master -- src/shop.c`**: **`BuyMenuDrawMapBg`** clamps metatiles with **`MapGridMetatileIdIsInEncodedSpace`** and uses **`MapGridMetatileIdUsesPrimaryTileset`** / **`MapGridMetatileNonPrimaryRowOffset`** instead of pret’s **`NUM_METATILES_IN_PRIMARY`** split — **intentional** map-grid / metatile encoding helpers. **Conclusion:** neither TU is a **safe bounded pret reconciliation** target on par with prior wins; **defer** for **`.text`** shaving. **No patch.**

**Audit-only pass (`teachy_tv.o`, pinned pret map):** Fork **`0x1720`** vs pret **`0x16e8`** (**`+0x38`**). Queue rank: after **`overworld.o` (`+0x10`)**, **`battle_setup`**, **`shop`**, **`teachy_tv`** is the next **shared** positive before **`save.o`**. **`git diff pret/master -- src/teachy_tv.c`**: (1) **`#ifdef PORTABLE`** **`Route1`** layout via **`Overworld_GetMapHeaderByGroupAndId`** + **`FireredPortableEffectiveMapLayoutForLayoutId`** — **zero** vanilla **`.text`** cost when **`PORTABLE`** unset. (2) **`MAP_LAYOUT_METATILE_MAP_PTR(layout)`** — on vanilla **`#else`** in **`map_layout_metatiles_access.h`** is **`(layout)->map`**, same as pret. (3) **`MAP_BG_PRIMARY_TILESET_TILE_COUNT`** / **`MAP_BG_SECONDARY_TILESET_TILE_COUNT`** — **`include/fieldmap.h`** defines primary as **`(NUM_TILES_IN_PRIMARY)`**; same wire sizes as pret’s **`NUM_TILES_IN_PRIMARY`** / **`NUM_TILES_TOTAL - NUM_TILES_IN_PRIMARY`**. (4) **`MapGridMetatileIdIsInEncodedSpace`** normalization to **`0`**, **`MapGridMetatileIdUsesPrimaryTileset`**, **`MapGridMetatileNonPrimaryRowOffset`** in **`TeachyTvLoadBg3Map`** and **`TeachyTvPushBackNewMapPalIndexArrayEntry`** — **Project C** policy documented in **`include/fieldmap.h`** (explicitly lists **Teachy TV** previews with **shop**). **Conclusion:** the **`+0x38`** is **intentional** fork metatile / layout plumbing, not accidental pret drift; **no bounded pret-shaped patch** without dropping that policy. **No patch.**

**Audit-only pass (`save.o`, pinned pret map):** Fork **`0x111c`** vs pret **`0x10d4`** (**`+0x48`**). **`git diff pret/master -- src/save.c`**: (1) **`EnsureSaveBlock2ExternalToolFingerprint`** (**`unkFlag1`/`unkFlag2`**, zero **`unkFlag2`..`battleTower`** padding) + calls before **`HandleSavingData`** / **`LinkFullSave_Init`** / **`WriteSaveBlock2`** and after successful **`LoadGameSave`** — **documented PKHeX / external-tool** RAM layout (**`SaveBlock2+0xAC`** word **`1`**); complements the landed **`new_game`** cleanup (redundant **`Sav2_ClearSetDefault`** writes removed because this path already applies the pattern). (2) **`#ifdef PORTABLE`** **`STATIC_ASSERT`**s on **`SaveBlock*`** sizes/offsets — **no** vanilla **`.text`**. (3) **`PortableFlash_PatchPkhexFrLgFingerprintAllSlots`** on **`PORTABLE`** after save/load success — **not** in vanilla compare. (4) Fork **drops** pret’s **`REVISION >= 0xA`** **`sloopsvc.h`** **`svc_*`** flash/link branches (**`TryWriteSector`**, **`HandleReplaceSector`**, **`Task_LinkFullSave`**) in favor of the **cartridge flash** path — the **`+0x48`** is **net** (**fingerprint + hooks** minus **removed sloop**). **Redundancy check:** each **`Ensure*`** callsite targets a **distinct** entry (**full save**, **`WriteSaveBlock2`**, **link full save init**, **post-load RAM** before continued play / export); removing any mirrors the **`new_game`** mistake class (PKHeX sees wrong RAM until next save). **Conclusion:** **not** a safe bounded pret reconciliation target; **no patch.**

**Measured `overworld.o` impact (call-site guard — `FieldMapVerifyLogicalToPhysicalTileTable`):** Runtime verify is **Direction B / non-debug table validation**; vanilla ships **identity **`gFieldMapLogicalToPhysicalTileId`** with offline **`tools/validate_field_map_logical_to_physical_inc.py`**. **`InitOverworldBgs` / `InitOverworldBgs_NoResetHeap`** call the verifier only under **`#ifdef PORTABLE`** (**`src/overworld.c`**, **`src_transformed/overworld.c`**). **`overworld.o(.text)`** **`0x3e14` → `0x3e0c`** (**`−0x8`**); aggregate **`.text`** **`0x15fa78` → `0x15fa60`** (**`−0x18`**); **`gMonFrontPicTable`** **`0x08235170` → `0x08235158`**; **ROM SHA1** **`c8c83c323c96928431a2467341573d3a0d74efa6`**.

**Measured linkage pass (`field_map_tile_logical_to_physical` verifier body):** **`fieldmap.h`** declares **`FieldMapVerifyLogicalToPhysicalTileTable`** only when **`PORTABLE && !NDEBUG`**; otherwise **`static inline`** no-op. **`field_map_tile_logical_to_physical.c`** defines the real body under the same guard — **vanilla** **`MODERN=0`** drops the symbol (**`arm-none-eabi-nm pokefirered.elf`** has **no** **`FieldMapVerify`**); **`field_map_tile_logical_to_physical.o` `.text`** **`0x174` → `0x5c`**; **late `extra` `.text.__stub` `0x20`** removed (**`memset` / `AGB_ASSERT`** veneers). **`gMonFrontPicTable`** unchanged at **`0x08235158`**; headline **`^\.text 0x08000000`** **`0x15fa60`** unchanged; **ROM `Used Size`** **`15406690` → `15406268`**; **ROM SHA1** **`e67bf89e8f766730eb7f7e6fb74aa6e4b3167130`**.

**`overworld.o` experiment (historical, still valid guidance):** Replacing **`BindMapLayoutFromSave()`** with pret’s **`gMapHeader.mapLayout = GetMapLayout()`** on vanilla **increased** **`.text`** and pushed **`gMonFrontPicTable` the wrong way** — **do not** repeat that slice without a new codegen hypothesis.

**Final `overworld.o` audit (lane close-out):** Fork vs pret **`src/overworld.c`** (**`git diff pret/master -- src/overworld.c`**) shows many hunks, but **`map_header_scalars_access.h`** macros collapse to **`(fallback)`** on vanilla — **no** extra **`.text`** there. The measurable residual (**`+0x10`** on **`overworld.o(.text)`**, pret pin **`0x3dfc`** vs fork **`0x3e0c`**) tracks the **save-authoritative layout bind**: **`BindMapLayoutFromSave`** (**`~0x20`** in **`agbcc`** output) plus **three** **`bl`** sites vs pret’s three inlined **`gMapHeader.mapLayout = GetMapLayout()`** tails, net of other codegen moves (link-facing table split, palette gamma loop via **`FireredPortableFieldMapBgPalettePrimaryRows()`**, return-type / pointer style cleanups). **Bounded trial (reverted):** in **`LoadCurrentMapData`**, after **`gSaveBlock1Ptr->mapLayoutId = gMapHeader.mapLayoutId`**, using pret’s direct **`GetMapLayout()`** assign on vanilla is **semantically redundant** with **`BindMapLayoutFromSave`** but **`agbcc`** grew **`LoadCurrentMapData`** (**`0x44` → `0x48`**) and **`overworld.o` `.text`** (**`0x3e0c` → `0x3e10`**). **Lane conclusion:** treat **`overworld.o +0x10`** as **accepted fork cost** for unified **`mapLayoutId` / `mapLayout`** binding and **PORTABLE** alignment; **no further overworld-specific bounded pret shave** unless a new codegen hypothesis appears.

**Category:** **Fork intentional / structural code drift** (extra TU splits, UI/window/sprite/list changes, metatile helpers, **`save.o`** fingerprint, linker stubs) compressing the ROM’s **`.text` + `.rodata`** boundary vs pret — **not** **`src/data.c`** monster graphics tables vs pret, and **not** a pret **`data.o`** prefix “hole.”

**Bounded `.text` reconciliation (landed + verified, cumulative):** **`window.o`**, **`pokemon.o`**, **`map_event_ids.h` + `field_specials`**, **`new_game` / `save` comment**, **`battle_ai_script_commands`**, **`event_object_movement`** — see rows above. **Object matches pret:** **`window`**, **`pokemon`**, **`field_specials`**, **`new_game`**, **`battle_ai_script_commands`**, **`event_object_movement`**.

**Toolchain / `.text.__stub` audit (this pass):** Fork map showed **two** **`.text.__stub`** regions (**`0x18`** before **`script_data`**, **`0x20`** in **`extra`**) when **`field_map_tile_logical_to_physical.o`** still contained **`FieldMapVerifyLogicalToPhysicalTileTable`** (**`memset` / `AGB_ASSERT`** pulled **`__memset_veneer`** / **`__AGBAssert_veneer`**). **Linkage pass (after overworld call-site guard):** **`fieldmap.h`** + **`field_map_tile_logical_to_physical.c`** compile the verifier body only under **`defined(PORTABLE) && !defined(NDEBUG)`**; vanilla **`MODERN=0`** has **no** **`FieldMapVerifyLogicalToPhysicalTileTable`** symbol (**`nm`** clean). **`field_map_tile_logical_to_physical.o` `.text`** **`0x174` → `0x5c`**; **late `extra` `.text.__stub` `0x20` removed**; first stub **`0x18`** unchanged (**`FieldMapFieldTileUploadUsesIdentityLayout`** veneer + **`Daycare_GetEggSpecies`**). **ROM `Used Size`** dropped **`15406690` → `15406268`** (**`−0x1a6`** bytes); headline **`^\.text 0x08000000`** stays **`0x15fa60`**. **`ld_script`** main-**`.text`** insertion remains **deferred** under headline metric (prior note).

**Audit-only pass (shared `src/*.o` `.text` queue close-out):** Pair every **`src/*.o`** **`.text`** size in **fork `pokefirered.map`** with **pret pin `pret-baseline-9bb/pokefirered.map`** where **both** exist; count **`fork > pret`**. **Result: exactly seven** shared positives, ranked by **Δ**: **`field_camera.o` (`+0x5d0`)**, **`fieldmap.o` (`+0x324`)**, **`save.o` (`+0x48`)**, **`teachy_tv.o` (`+0x38`)**, **`shop.o` (`+0x24`)**, **`battle_setup.o` (`+0x20`)**, **`overworld.o` (`+0x10`–`+0x14` depending on link snapshot)**. The **two largest** had not been narrative-audited in this doc before; **`git diff pret/master -- src/field_camera.c`** / **`src/fieldmap.c`** show **only** **Project C / Direction B** plumbing (metatile encode + tilemap translation; block words on map load / connections / saved view). **Conclusion:** **no** candidate in the seven is a **clean accidental pret slice** comparable to the landed **`window` / `pokemon` / …** wins; **bounded source-level pret-shaped `.text` reconciliation is effectively done** unless map policy or toolchain/metric policy changes.

**Next lane direction (post-queue):** Superseded by **§ Lane state**, **§ Success criteria**, and **§ Metrics policy** — optional **`PORTABLE`** / **`ld_script`** / compare-charter work only with an **explicit** decision.

---

## 1. What the reference lane is / is not

| In scope | Out of scope (this lane) |
|----------|---------------------------|
| Clear baseline for **vanilla GBA** build health | Forcing **full `make compare_*` green** while fork layout/data intentionally differs |
| Cataloging **drift categories** and ownership | **Project A/B/C** map-block-word runtime, SDL/Godot architecture |
| **Documented** pret sync workflow (`git` remote, diffs) | Rewriting portable runtime to match pret file layout |
| **Lane state / metrics** (**§ Lane state**, **§ Metrics policy**) | Ongoing **pret-shaped `.text` queue** work without a **new charter** |

**Success:** Anyone can answer: “Is this delta **intentional fork**,” **accidental pret drift**, **generated data**, **build-rule** drift, or **structural Project C** — and where to look next. **Merge / release gates** for this fork **do not** assume compare green unless explicitly adopted (**§ Success criteria**).

---

## 2. Reference baseline (artifacts)

These define the **vanilla binary reference** for FireRed (rev0 English in-tree naming):

| Artifact | Role |
|----------|------|
| **`baserom.gba`** | Retail ROM (local only; not in git). Same size as built ROM (`0x1000000`). |
| **`firered.sha1`** | Expected SHA1 of **`pokefirered.gba`** when **`COMPARE=1`** (`make compare_firered`). |
| **`pokefirered.gba`** | Output of **`arm-none-eabi-objcopy`** from **`pokefirered.elf`** after **`gbafix`**. |
| **`pokefirered.map`** | Symbol / section placement; use to map ROM offsets → **object / section**. |
| **pret `pokefirered`** | **Vanilla** C/data/jsonproc reference — **pin** in **§0**; add/update remote and diff commands in **`upstream_pokefirered.md`** §1. |

---

## 3. Drift inventory (categorized)

### A. Should match pret for a credible vanilla reference lane

Use pret (or regenerated jsonproc) when the goal is **GBA decomp fidelity** for shared files:

- **Jsonproc / generated headers** under **`src/data/`** (e.g. `items.h`, `wild_encounters.h`, `src/data/region_map/*.h`) — must come from **JSON + templates**, not portable `#include` stubs that hide symbols from **agbcc**.
- **Core decomp sources** you intend to treat as vanilla: **`src/*.c`**, **`include/*.h`** for the non-`PORTABLE` path.
- **Pret-agreed ABI details:** e.g. **`struct Sprite.template`**, **`include/malloc.h`** heap API, **`STATIC_ASSERT`** vs `_Static_assert` for **agbcc**.

### B. Intentional fork / portable divergence (do not “fix” toward pret blindly)

- **`#ifdef PORTABLE`** branches, **`cores/firered/portable/`**, **`src_transformed/`**, **`engine/`**.
- **Extended save / map / tooling** documented under **`docs/architecture/`** (e.g. block-word save field, layout bins).
- **Daycare split** (**`daycare_egg_*.c`**) + **`agbcc_arm`** for **`daycare_egg_species.o`** — toolchain workaround; **thumb `agbcc` ICE** if merged; see **`upstream_pokefirered.md` §5**. This is a **compare risk**, not a runtime bug by itself.

### C. Generated-data drift

- Regenerate with **`make generated`** or targeted **`make src/data/...`** when JSON or templates change.
- Symptom: link errors for missing symbols from **empty/wrong** generated headers.

### D. Build-rule / environment drift

- **Makefile** `INCLUDE_DIRS`, **`MODERN`**, **`COMPARE`**, **`GAME_*`**.
- **Windows:** use **MSYS2 MinGW64 + `mingw32-make`** so **`SHELL` / `uname`** and **agbcc** paths match pret’s documented flow.

### E. Unclear / needs explicit decision

- Any change that **moves rodata/text** (e.g. first ROM diff in **`rom_header_gf.o`** pointer to **`gMonFrontPicTable`**) — usually means **global link layout** differs from retail; treat as **fork-wide layout delta** until map/compare against pret proves otherwise.
- **Single-TU vs split TU** (daycare): same **symbols at link**, but **section order** may differ vs pret → **SHA** mismatch even if logic matches.

---

## 4. Diagnosing SHA / ROM mismatch (reference lane, not blame)

1. **`make compare_firered`** → if **`pokefirered.gba: FAILED`**, the ROM differs from **`firered.sha1`**.
2. **Byte diff** **`baserom.gba`** vs **`pokefirered.gba`** (first offset): map with **`pokefirered.map`** to **section + `*.o`**.
3. **Example recorded in §0.1** — first divergence at **`0x08000128`** / **`gMonFrontPicTable`** pointer; widespread remaining diffs ⇒ **layout/content fork drift**, not a single-header typo.

---

## 5. Bounded reconciliation workflow (recommended order)

**Context:** The **pret-shaped bounded `.text` queue** is **closed** (**§ Lane state**). Use this workflow for **pret merges**, **jsonproc refresh**, and **diagnosis** — not for routine map-diff byte mining.

1. Follow **`upstream_pokefirered.md`** §2–§3 for **jsonproc regeneration** and **include paths**.
2. **`git fetch pret`** periodically and diff **Makefile**, **`include/`**, **`ld_script.ld`**, **`sym_*.txt`**, **`src/data/`** generators (against **§0** commit or **`pret/master`** after fetch).
3. For a **symbol placement** question, diff **`pokefirered.map`** vs a **pret**-built **`.map`** (same **`MODERN=0`**).
4. Treat **compare green** as a **separate goal** from **fork main** health unless the team explicitly prioritizes SHA parity (**§ Success criteria**).

---

## 6. Next steps (policy / optional work)

- **Re-pin pret (when advancing the baseline):** `git fetch pret`, update **§0** with new **`pret/master`** tip **commit** + **date** if you intentionally track upstream movement.
- Optional **CI**: archive **`pokefirered.map`** (and optionally ROM) for **vanilla-oriented** branches.
- If **bit-identical compare** is required: scope a **dedicated** effort (branch or charter) — **not** an extension of the **closed** bounded queue.
- **Metric / `ld_script` policy:** only with an explicit decision (**§ Metrics policy**, **§ Lane state** — deferred toolchain row).
