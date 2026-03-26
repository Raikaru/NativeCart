# CFRU integration playbook (native / portable FireRed fork)

Planning doc for bringing **Complete Fire Red Upgrade (CFRU)**–class behavior into this tree **without GBA emulation**. It complements the ROM-backed matrix in **`docs/rom_compat_architecture.md`**, the portable stabilization notes in **`docs/portable_runtime_notes.md`**, and the **lane-stop / compare-policy** notes in **`docs/upstream_reference_lane.md`**.

**Upstream links, build steps, and compliance pointers:** **`docs/cfru_upstream_reference.md`**.

## 1. What CFRU is (and what this repo is not)

| Aspect | CFRU upstream | This fork |
|--------|----------------|-----------|
| **Primary artifact** | Patched **BPRE** ROM via insertion scripts (`scripts/make.py`, `offsets.ini`) | **Native portable runtime** (`engine/`, `cores/firered/`) + optional loaded `.gba` for **ROM-backed tables** |
| **Battle engine** | Upgraded toward **Gen 8** (moves, abilities, items, AI, megas, Z-moves, terrain, etc.—see CFRU README feature list) | **Vanilla FireRed-shaped** battle core (`src/battle_*.c`, `gBattleScriptingCommandsTable`, …) unless extended here |
| **Data scale** | Expanded species / moves / text buffers / save (config-driven) | **`NUM_SPECIES` / `MOVES_COUNT` / generated data** still **vanilla-sized** unless widened and regenerated |
| **Source coupling** | **GBA** Thumb/C/ASM tied to ROM layout | **PORTABLE** path uses **fake GBA map + software PPU** (`engine/core/engine_internal.h`, `engine_renderer_backend.c`, …) |

CFRU is **not** a drop-in library for a pret-style decomp: it is a **ROM upgrade** you either **merge at binary/tooling level** or **reimplement semantically** in C.

## 2. Goals and non-goals

**In scope**

- Treat CFRU as a **behavioral and data-layout reference** for a **compatibility profile** (vanilla vs CFRU-shaped hacks).
- Extend **battle**, **scripts**, **save**, and **table loaders** so a **declared target** (e.g. a specific Unbound revision) can run **natively** to a defined parity bar.
- Reuse existing **ROM overlay** machinery (`firered_portable_rom_*`, resolver tokens—see **`docs/rom_compat_architecture.md`**) where CFRU still uses **FireRed-like** flat tables and **`0x08……`** pointers.

**Out of scope (unless the project explicitly changes direction)**

- **Executing arbitrary patched ROM machine code** (full fidelity for every PokeCommunity patch without a profile).
- **Importing CFRU `.s` blobs** into the portable build without a **host-side** execution story.
- Promising **byte-identical** behavior without a **parity harness** (battle traces, scripted scenarios).
- Generic support for **custom UI engines**, **custom menu rewrites**, **custom field systems**, or **bespoke facility logic** unless a target profile explicitly adopts them.

## 3. Entry gate and program boundary

Treat CFRU work as a **new chartered compatibility program**, **not** as a continuation of the now-closed bounded pret reconciliation lane in **`docs/upstream_reference_lane.md`**.

**Prerequisites before serious CFRU work**

- **Portable base product stability first**: SDL / native runtime should **build, boot, save, load, and battle sanely** on vanilla FireRed before CFRU-class behavior is treated as an active target.
- **Known baseline workflows** should already be trustworthy enough that a regression can be blamed on CFRU-facing work, not on generic startup / save / timing instability.
- **Parity harness or scenario harness** should exist before large semantic merges, even if it starts narrow.

**What must be explicit before a CFRU push starts**

- **Target**: single-hack profile, CFRU baseline, or a declared compatibility tier.
- **Success bar**: boot only, field + save, battle slice, or broader playable milestone.
- **Non-goals**: what the current push will **not** attempt to support (for example custom UI engines, bespoke field systems, or undocumented ASM-only behavior).

Without that charter, CFRU work tends to become undirected battle/script churn on top of an unstable base.

## 4. Where CFRU pressure lands in this codebase

| Layer | Repo location | CFRU relevance |
|-------|----------------|----------------|
| **Shell / presentation** | `engine/shells/sdl/` | Low for CFRU **semantics**; already presents `engine_video_get_frame()`. |
| **AGB bridge (I/O, timing, PPU)** | `engine/core/engine_renderer_backend.c`, `engine_input_backend.c`, `engine_platform_portable.c`, `engine_runtime_backend.c` | **Medium**: IRQ/VBlank cadence, scanline effects, input; **not** the main CFRU battle gap. |
| **ROM load + compat fingerprint** | `cores/firered/portable/firered_portable_rom_compat.c` | **High for UX**: known profiles (e.g. Unbound hints); does **not** implement CFRU mechanics. |
| **ROM-backed tables** | `cores/firered/portable/firered_portable_rom_*.c`, `docs/portable_rom_*.md` | **High** for **data** that stays **tabular** in the hack ROM; **low** for new opcodes and battle script semantics unless paired with runtime changes. |
| **Battle core** | `src/battle_main.c`, `src/battle_script_commands.c`, `src/battle_util.c`, `include/constants/battle_script_commands.h`, … | **Critical**: CFRU’s bulk is **battle + AI + commands**. |
| **Overworld scripts** | `scrcmd.c`, `event_scripts`, `portable_script_pointer_resolver.c` | **High** for CFRU **new specials** and bytecode differences. |
| **Compiled / generated data** | `pokefirered_core/generated/`, `cores/firered/generated/data/`, `tools/portable_generators/` | **Critical** when **species/move/item counts** or struct layouts change. |
| **Assets** | `src_transformed/*_portable_assets.h`, `PORTABLE_FAKE_INCBIN`, `graphics/` | **High** for new species, forms, UI, animations. |
| **Save** | `src_transformed/agb_flash.c`, flash layout | **High** for CFRU **save expansion** and hack-specific chunks. |

## 5. Blocker inventory (typical order of attack)

1. **Parity definition** — “Boot title” vs “full story” vs “competitive battle matrix”; without this, scope is undefined.
2. **Save format** — size, checksums, new fields; portable flash path must match target hack or later tests become misleading.
3. **ID space and structs** — `NUM_SPECIES`, `MOVES_COUNT`, party/box structs, summary buffers; **generators** and **ROM packs** must agree or you get silent corruption.
4. **Battle script command table** — `gBattleScriptingCommandsTable`, `RunBattleScriptCommands`, move/ability/item effect handlers; CFRU extends all of these.
5. **Type chart / species info / moves / learnsets** — may load from ROM **if** layout matches validators; CFRU often **changes** layouts vs vanilla—**profile-specific** decoders may be required.
6. **Overworld script commands and specials** — new `scrcmd` behavior; must match hack bytecode or events stall.
7. **Graphics and audio** — new mon/anim/UI; **INCBIN** and `_Portable` fallbacks must be extended or ROM-bound.
8. **Hardcoded C seams** — e.g. main menu path documented as **not ROM-driven** in **`docs/portable_runtime_notes.md`**; CFRU-style hacks expect **ROM-driven flow** in many places—each seam is a **compatibility** decision.
9. **GBA ROM build (`make compare_firered`)** — if red, you cannot use “merge CFRU into ELF” as a shortcut; **portable C** remains the integration path (see **`docs/upstream_reference_lane.md`** for policy on pret comparison).

## 6. CFRU compatibility layer (recommended shape)

Use a **profile-gated adapter** so `PORTABLE` does not accumulate `if (unbound)` across the tree.

| Piece | Role |
|-------|------|
| **Profile registry** | Select `vanilla_firered`, `cfru_baseline`, or per-hack ids (aligned with **`FireredRomCompatInfo`** / `profile_id` in **`docs/portable_rom_compat.md`**). |
| **Table decoders** | Parse **ROM or extracted** blobs into **engine-neutral** structs; **validate** bounds before activating pointers. |
| **Script dispatch extensions** | Extra **map** and **battle** opcodes live in **tables** keyed by profile, not scattered `switch` edits. |
| **Battle feature bundle** | Register **move/ability/item** handlers and **turn pipeline** hooks required by the active profile. |
| **Explicit non-goals** | Document per profile what is **not** emulated (e.g. undocumented ASM-only behavior, custom UI/menu engines, bespoke field subsystems). |

The layer **organizes** CFRU work; it does **not** remove the need to implement **battle semantics**.

## 7. “Unbound only” vs “general CFRU”

| Strategy | Schedule impact | Maintenance |
|----------|-----------------|-------------|
| **Single-hack target** (e.g. one Unbound revision) | Usually **faster** to first playable: **special-case** loaders and fewer abstraction constraints. | Higher **migration cost** if you later add Radical Red–class targets. |
| **CFRU-shaped platform** | More **up-front** design (profiles, tests, shared decoders). | Easier to add **second** CFRU derivative **if** battle core is truly shared. |

Heavy hacks built on CFRU still differ in **per-game** scripts, data repoints, and UI—profiles must stay **honest**.

## 8. External “bridge” references (hardware only)

[AntonioND/libugba](https://github.com/AntonioND/libugba) is a useful **pattern** for **SDL vs hardware** separation (`source/sdl2`). This fork already centralizes much of that in **`engine/core/`** (see §F of **`docs/rom_compat_architecture.md`**). **Do not** expect libugba to substitute for **CFRU battle** work.

**Community Emerald PC ports** (e.g. **`pc-port`**-style branches on **`pokeemerald` forks**) are fair **second-hand references** for **host shell**, **timing**, and **platform shim** ideas on **another Gen III decomp**. **Emerald is not FireRed**—use for **structure**, not file-level merges. The **canonical place** for this in the **ROM-backed / portable** doc set is **`docs/rom_compat_architecture.md`** §G and the intro of **`docs/rom_backed_runtime.md`**; URLs and caveats stay in **`docs/cfru_upstream_reference.md`** §6 so one link target does not drift.

## 9. Related documentation

| Doc | Why read it |
|-----|-------------|
| **`docs/cfru_upstream_reference.md`** | CFRU/DPE repos, wiki, ROM insertion build model, `config.h`, pret vs CFRU, optional **Emerald `pc-port` fork** reference (§6). |
| **`docs/rom_compat_architecture.md`** | ROM load path, overlay priority, resolver tokens, AGB-bridge notes. |
| **`docs/portable_rom_compat.md`** | `profile_id`, flags, known hack rows. |
| **`docs/rom_backed_runtime.md`** | Per-vertical ROM migration and fallbacks. |
| **`docs/portable_runtime_notes.md`** | SDL validation, save path env vars, main menu ROM seam, `INCBIN` failure modes. |
| **`docs/generated_data_rom_seam_playbook.md`** | Scale and risk of `map_data_portable` / generated data. |
| **`docs/architecture.txt`** | Engine vs core vs shell split. |
| **`docs/upstream_reference_lane.md`** | pret baseline / `compare_*` policy. |

## 10. Suggested first milestones (engineering)

1. **Parity harness skeleton** — scripted battle steps or trace compare strategy (even stubbed) before large merges.
2. **Profile flag** — `cfru_like` (or per-game) that toggles **one** vertical (e.g. type chart + species info ROM decode only) end-to-end.
3. **Save + main menu audit** — list **hardcoded** vanilla paths that contradict loaded ROM; decide **ROM-backed** vs **explicit unsupported** before deeper battle claims.
4. **Battle command slice** — implement **one** CFRU-visible behavior with tests (e.g. a single modern move effect class) to measure **integration cost**.

## 11. CFRU README feature list (planning mirror)

The upstream README advertises (non-exhaustive): **expanded PC**, **Gen 8–style battle** (moves, abilities, items, AI, animations, Z-Moves, Mega/Primal/Ultra, hidden abilities, terrain, totems, inverse/sky battles, multi battles, etc.), **frontier/facilities**, **TM/HM expansion**, **DexNav**-class tools, **save expansion**, **day/night/seasons**, **roaming**, **JPAN-era** features (customization, scripting specials), **fairy type**, **follow me**, **expanded Hall of Fame**, **new field moves**, and many **QoL** scripts. Treat this as a **checklist surface** when estimating **native** port work—not as a promise that each item maps cleanly to a single ROM table.
