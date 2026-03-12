# AGENTS Guide

This repository is a portable-runtime + Godot host source port of Pokemon FireRed.
The project compiles the pokefirered decompilation as native C on a 64-bit Windows host,
wraps it in a Godot 4.x GDExtension, and runs the game in real time via a frame-step
threading model. There is no emulator — GBA hardware is replaced by portable memory
arrays and native rendering.

## Architecture snapshot
- Treat the FireRed engine as the primary codebase.
- Keep the portable-C runtime path intact.
- Keep the Godot layer as a thin host shell, not a second gameplay implementation.
- Preserve ROM-backed asset access and patch-overlay behavior.
- Prefer restoring real production paths over adding temporary stubs.
- Diagnose linker or integration failures by object family first, not symbol by symbol.
- All portable-mode changes must be guarded by `#ifdef PORTABLE` so the ROM build is unaffected.

## How the runtime works
- Two threads: the main/Godot thread calls `firered_run_frame()` which signals a frame request and waits for `completed_frames` to increment. The engine thread runs the game loop (AgbMain → main loop → callbacks → WaitForVBlank).
- Each frame, the engine thread runs game logic, then hits `firered_runtime_vblank_wait()` which increments `completed_frames` and waits for the next frame request.
- The Godot scene calls `step_frame()` once per `_process()` tick, fetches the framebuffer texture, and assigns it to a TextureRect.
- The renderer in `firered_video.c` reads from portable memory arrays (VRAM, PLTT, OAM, IO registers) and composites a 240x160 RGBA framebuffer.
- Manual play uses `scripts/play_session.gd` with keyboard input mapped to GBA buttons. Automated regression uses `scripts/runtime_smoke_test_auto.gd` with scripted input and frames_per_tick=20 speedup.

## GBA hardware memory map in portable mode
The GBA's memory-mapped hardware is replaced by flat arrays. These are the critical
address ranges and their portable equivalents:

    0x05000000  PLTT (palette RAM, 1 KB)   -> portable palette array in firered_video.c
    0x06000000  VRAM (video RAM, 96 KB)    -> portable VRAM array in firered_video.c
    0x07000000  OAM  (sprite attrs, 1 KB)  -> portable OAM array in firered_video.c
    0x04000000  IO registers               -> portable IO register array

Any code that writes to these hardware addresses (DMA, direct writes, BIOS calls)
must resolve to the portable arrays under PORTABLE mode. This is the single most
common source of bugs in this project.

## Agent model tiers and task routing

This project uses a hybrid workflow. Different models handle different task types.
Read this section to understand what kind of task you are doing and how to approach it.

### Tier 1 tasks — mechanical, known-pattern (Kimi / lighter model)
These tasks follow patterns already solved in this codebase. Apply the fix pattern
from the "Known bug families" section below. Do not spend time re-diagnosing the
root cause — it is already understood.

Examples of Tier 1 tasks:
- "This tileset INCBIN is null in portable mode" → apply the embedded byte array
  pattern from existing *_portable_assets.h files
- "This function has a GBA address range check that rejects host pointers" → add
  the PORTABLE non-NULL guard, same as IsTileMapOutsideWram fix in bg.c
- "This DMA call targets a hardware address" → resolve through the portable memory
  map, same as dma3_manager.c fix
- "Embed raw bytes for these 20 assets" → extract from binary files, format as C
  arrays, add to the appropriate portable assets header
- Grep/audit tasks: "find all INCBIN assets in src_transformed/fieldmap.c" or
  "find all GBA address range checks in src_transformed/"
- Bulk data conversion and code generation

How to handle Tier 1 tasks:
- Skip diagnosis. The bug family is identified in the prompt.
- Apply the fix pattern to ALL instances, not just the one named in the prompt.
- Build, run, report results. One round.

### Tier 2 tasks — diagnosis required, new instance of known family (Codex / stronger model)
The bug is new but may match a known family. Diagnosis is needed to confirm which
family it belongs to and find the exact failure point.

Examples of Tier 2 tasks:
- "The game crashes when entering a new area" → could be INCBIN null, DMA, pointer-
  width, or address range check. Need log output to determine which.
- "A new script command crashes" → could be pointer resolution or a new command that
  needs a portable implementation.
- "Battle rendering is broken" → new system, but individual sub-bugs likely match
  known families.

How to handle Tier 2 tasks:
- Diagnose narrow: add targeted prints, build, run, report.
- Once the family is identified, fix broad: apply to all instances.
- If the diagnosis reveals a known family, switch to Tier 1 execution.
- Up to 2-3 diagnosis-fix cycles per task.

### Tier 3 tasks — genuinely new bug family (Codex / strongest model only)
The bug does not match any known family. A new fix pattern needs to be designed.

Examples of Tier 3 tasks:
- The battle engine's state machine has a fundamentally different threading
  assumption than the overworld.
- A new hardware subsystem (sound, timers) needs portable stubs designed from
  scratch.
- Struct layout divergence between 32-bit and 64-bit that affects save data
  interpretation across multiple systems.

How to handle Tier 3 tasks:
- Invest in thorough diagnosis before attempting any fix.
- Design the fix pattern carefully — it will become a new entry in the known
  bug families section.
- Document the new pattern in this file after the fix is proven.

### When in doubt about tier
If the prompt does not specify a tier, check the symptoms against the known bug
families below. If any family matches, treat it as Tier 1. If no family matches
but the symptoms are clear, treat as Tier 2. Only escalate to Tier 3 if diagnosis
reveals something genuinely novel.

## Diagnosis and fix philosophy

### Diagnose narrow, fix broad
When diagnosing a new bug, take the smallest possible step to identify the root cause.
Add targeted prints, build, run, report. Do not guess or apply speculative fixes.

When fixing a bug that matches a pattern already solved elsewhere in this codebase,
fix the ENTIRE CATEGORY in one pass rather than the single instance. Look for every
other place the same pattern will break and fix them all together.

When adding a new system or stub, consider what the game will need in the next 5-10
progression steps, not just the immediate one. If you are stubbing out a hardware
system, stub the complete interface, not just the one function that crashed.

### Batched bug fixes
The human operator may play the game manually and collect multiple bugs before
sending a prompt. When a prompt contains multiple bugs:
- Classify each bug against the known families
- Fix all bugs that match known families in one pass
- If any bug requires diagnosis, do that last
- Report all results together

### Known bug families and their fix patterns
These are recurring bug categories. When you encounter one, apply the fix pattern to
ALL instances in the codebase, not just the one that triggered the current failure.

**1. DMA to hardware address doesn't land in portable mode**
- Symptom: game data is correct in source buffers but destination hardware memory stays
  at default values (0x7FFF for palette = white, 0x0000 for VRAM = empty).
- Root cause: DmaCopy16/DmaCopy32/DmaFill macros or RequestDma3Copy/RequestDma3Fill
  target GBA hardware addresses that don't resolve to portable arrays.
- Fix pattern: in the DMA processing path, resolve hardware destinations to portable
  arrays and perform memcpy/CpuCopy16 instead. Mark requests complete immediately.
  Do not leave them pending waiting for hardware DMA completion signals.
- Status: FIXED comprehensively. TransferPlttBuffer in palette.c uses CpuCopy16 for
  PLTT. dma3_manager.c resolves all queued DMA to 0x05/0x06/0x07 ranges. Direct DMA
  macros use CpuSet under PORTABLE via include/gba/macro.h.
- Scope rule: when fixing one DMA path, audit ALL DMA paths for the same issue.

**2. INCBIN assets null or wrong in portable mode**
- Symptom: LoadPalette/LZ77UnCompVram receives a null or garbage pointer. Game stalls
  or renders black/white because asset data never reaches the buffers. Also manifests as
  size=0 DMA transfers when tileset graphics are null.
- Root cause: INCBIN macro produces a different representation in portable mode. The
  pointer either resolves to null or to a buffer with wrong contents.
- Fix pattern: bypass the transformed INCBIN with explicit raw embedded byte arrays
  under PORTABLE. Extract bytes from the original binary files in gfx/data dirs,
  convert to C arrays, add to the portable assets header, bind pointers under PORTABLE.
- Already fixed:
  - Copyright screen: sCopyright_Gfx, sCopyright_Map in intro_portable_copyright_assets.h
  - Intro scenes: Scene2/Scene3 palettes in intro_portable_copyright_assets.h
  - Font/keypad glyphs: all font tables in text_portable_assets.h
  - Overworld sprites: object event graphics in object_event_graphics_portable_assets.h
  - Indoor tilesets: gTileset_Building, gTileset_GenericBuilding1 in tilesets_portable_assets.h
- Still needed: outdoor tilesets (gTileset_General, gTileset_PalletTown, and all other
  map-specific secondary tilesets). Any new area visited may surface more null tilesets.
- Scope rule: when one INCBIN asset is null, audit ALL INCBIN assets in the same file
  and fix them all in one pass. Do not fix them one at a time across multiple rounds.
- Tier 1 indicator: if the prompt says "this tileset/sprite/palette INCBIN is null",
  skip diagnosis and apply the embedded array pattern directly.

**3. Pointer-width bugs on 64-bit host**
- Symptom: crash or corruption when game code stores a host pointer in a 16-bit or
  32-bit field (task args, script data, callback storage).
- Root cause: GBA is 32-bit. Some game code packs pointers into u16 task args or
  stores them via 32-bit unsigned long APIs. On a 64-bit host, pointers are 8 bytes.
- Fix pattern: use pointer-width-safe storage under PORTABLE. For task args, use
  side storage that holds full native pointers. For script operands, use a native-width
  reader. Preserve the original 32-bit path for ROM builds.
- Already fixed: task word args in task.c/task.h, script pointer operands in script.c.
- Scope rule: when one pointer-width bug appears, grep for similar patterns (casting
  function pointers to u32, packing pointers into u16 pairs) and fix them proactively.

**4. Hardware spin-loops that never resolve in portable mode**
- Symptom: engine thread hangs. Last trace line shows entry into a function that
  never returns.
- Root cause: GBA code spins waiting for a hardware condition (VBlank flag, DMA
  completion, link adapter response, RFU state) that the portable runtime doesn't
  provide.
- Fix pattern: add a PORTABLE early-return, no-op stub, or condition skip matching
  the pattern of existing fixes. Common offenders: RFU functions, link handling,
  soft-reset combo check, IntrWait/halt equivalents.
- Already fixed: soft-reset branch in main.c, various RFU/link stubs.
- Scope rule: when stubbing one hardware wait, check if the same subsystem has other
  wait functions that will stall next and stub them together.

**5. Display register / render state not flushed**
- Symptom: renderer sees stale or default register values even though game code set them.
- Root cause: register writes go through buffered paths (gGpuRegBuffer) that require
  explicit flushing via CopyBufferedValuesToGpuRegs() + ProcessDma3Requests().
- Fix pattern: ensure the frame-step path calls the full flush sequence before
  rendering. Currently the flush runs: gMain.vblankCallback() →
  CopyBufferedValuesToGpuRegs() → ProcessDma3Requests().
- Scope rule: if adding new rendering features, verify they read post-flush state.

**6. GBA address-range checks rejecting valid host pointers**
- Symptom: a function that validates memory addresses rejects heap-allocated buffers
  on 64-bit because they fall outside the GBA's IWRAM/EWRAM address ranges.
- Root cause: GBA code checks if a pointer falls within 0x02000000-0x0203FFFF (EWRAM)
  or 0x03000000-0x03007FFF (IWRAM). Host pointers on 64-bit are in a completely
  different range.
- Fix pattern: under PORTABLE, treat any non-NULL pointer as valid. Replace the
  address range check with a simple null check.
- Already fixed: IsTileMapOutsideWram() in bg.c.
- Scope rule: grep for IWRAM_END, EWRAM_END, IWRAM_START, EWRAM_START, and raw
  address comparisons like `ptr >= 0x02000000`. Fix ALL of them under PORTABLE.
- Tier 1 indicator: if you find an address range check, fix it immediately without
  diagnosis.

**7. Map event script pointers unresolved**
- Symptom: crash when the player triggers a map event (talks to NPC, steps on trigger,
  reads sign). The script pointer is a raw 0x08xxxxxx ROM address that gets
  dereferenced as a host pointer.
- Root cause: map event data stores script pointers as GBA ROM addresses. These need
  to go through the portable script pointer resolver before reaching
  ScriptContext_SetupScript.
- Fix pattern: resolve the script pointer through portable_script_pointer_resolver
  at the handoff point where map event data enters the script engine.
- Already fixed: ObjectEventTemplate.script, CoordEvent.script, BgEvent.bgUnion.script
  in event_object_movement.c and field_control_avatar.c.
- Scope rule: any new map event type or script source that reaches the script engine
  needs the same resolution. Check all callers of ScriptContext_SetupScript.

### Diagnostic best practices
- Always fflush(stdout) after every printf in diagnostic code. Without this, output
  may not appear before a hang and you lose the last trace line.
- Guard all diagnostics with `#ifdef PORTABLE` so they don't affect ROM builds.
- For frame-specific diagnostics, gate them to fire only at specific frame numbers
  (e.g. frame 16) rather than every frame. Per-frame prints cause timeouts.
- When tracing a hang, use before/after print pairs around every call in the suspect
  block. The last printed line = the line before the hang.
- When tracing data flow, dump the first 8 entries of each buffer in the pipeline
  to find where valid data stops and default values begin.

## Current project state

### What works
- ROM load, init, and full intro sequence (copyright, Game Freak logo, title screen)
- New Game initialization (quest log, save block, Pokemon storage)
- Map loading (all cases 0-11, including tileset copy, tilemap, BG setup)
- DMA for all hardware ranges (PLTT, VRAM, OAM) in both direct and queued paths
- Palette pipeline (load, fade, transfer to hardware)
- BG layer rendering (tilemaps, scroll, multiple layers)
- Sprite/OAM rendering (player, NPCs visible)
- Text rendering (font glyphs, dialogue boxes)
- Script execution (map scripts, event scripts, dialogue)
- Map transitions (stair warps confirmed working)
- Manual keyboard play with real-time 60fps display
- Automated regression smoke test with frames_per_tick=20 speedup

### Current blockers / known issues
- Outdoor tileset INCBIN assets load with size=0 (gTileset_General, gTileset_PalletTown)
  causing crash when transitioning to Pallet Town outdoor map. Known INCBIN family.
- Dialogue box border has minor tile corruption (cosmetic, low priority)
- BG scroll edge wrapping shows black columns at map edges (cosmetic, low priority)
- Metatile behaviors load correctly for indoor tilesets but need verification for
  outdoor tilesets once the tileset graphics are fixed
- Battle engine is entirely untested

### Game progression reached
- Title screen → New Game → bedroom → 1F (stair warp works) → door area
- Player and NPC sprites visible
- Text/dialogue working
- Next milestone: outdoor Pallet Town, then Oak cutscene, then starter selection,
  then rival battle

## Source map
- Core game/decomp code: `src/`, `src_transformed/`, `include/`, `asm/`, `data/`, `sound/`
- Portable runtime/static core: `pokefirered_core/`
  - `pokefirered_core/src/firered_runtime.c` — frame stepping, thread sync, engine init
  - `pokefirered_core/src/firered_video.c` — renderer, framebuffer compositing, portable VRAM/PLTT/OAM arrays
  - `pokefirered_core/src/portable_script_pointer_resolver.c` — ROM pointer resolution
- Godot host + GDExtension bridge: `gdextension/`
  - `gdextension/firered_node.cpp` / `.hpp` — FireRedNode GDExtension class
  - `gdextension/firered_renderer.cpp` — texture handoff to Godot
  - `gdextension/firered_extension_init.cpp` — extension registration
  - `gdextension/SConstruct` — scons build for the extension
- Transformed/portable game source: `src_transformed/`
  - `src_transformed/main.c` — main loop, callback dispatch
  - `src_transformed/intro.c` — intro sequence callbacks
  - `src_transformed/palette.c` — palette buffers, TransferPlttBuffer, LoadPalette
  - `src_transformed/dma3_manager.c` — DMA request queue processing
  - `src_transformed/overworld.c` — map loading state machine (LoadMapInStepsLocal)
  - `src_transformed/script.c` — script execution engine
  - `src_transformed/scrcmd.c` — script command handlers
  - `src_transformed/task.c` — task system
  - `src_transformed/gpu_regs.c` — GPU register buffering and flush
  - `src_transformed/bg.c` — BG layer management, tilemap loading, IsTileMapOutsideWram fix
  - `src_transformed/field_control_avatar.c` — warp checking, event dispatch
  - `src_transformed/fieldmap.c` — map data access, metatile behavior lookup
  - `src_transformed/event_object_movement.c` — object event scripts, sprite graphics
  - `src_transformed/text.c` — text rendering, font data
  - `src_transformed/pokemon_storage_system_menu.c` — PC storage system
  - `src_transformed/new_game.c` — New Game init sequence
  - Portable asset headers:
    - `src_transformed/intro_portable_copyright_assets.h`
    - `src_transformed/text_portable_assets.h`
    - `src_transformed/tilesets_portable_assets.h`
    - `src_transformed/object_event_graphics_portable_assets.h`
- Play session: `scripts/play_session.gd` (manual keyboard play)
- Automated smoke test: `scripts/runtime_smoke_test_auto.gd`
- Scenes: `scenes/runtime_smoke_test.tscn` (manual), `scenes/runtime_smoke_test_auto.tscn` (regression)
- Vendored Godot C++ bindings and tests: `godot-cpp/`
- CI/build references: `.github/workflows/`

## Key files by debugging context
When debugging a specific system, start by reading these files:

- **White/black screen**: firered_video.c (renderer), palette.c (transfer), dma3_manager.c (DMA)
- **Hang/stall**: firered_runtime.c (frame sync), main.c (main loop), the specific callback that's active
- **Null asset/crash on load**: the relevant *_portable_assets.h file, the file that includes the INCBIN
- **Map loading stall**: overworld.c (LoadMapInStepsLocal cases), dma3_manager.c, bg.c
- **Script crash**: script.c (execution), scrcmd.c (commands), the specific map script data
- **Task crash**: task.c (task system), include/task.h (pointer-width storage)
- **Warp doesn't trigger**: field_control_avatar.c (TryStartWarpEventScript, TryArrowWarp), fieldmap.c (metatile behavior), the map's metatile data
- **Tileset/map graphics missing**: tilesets_portable_assets.h, fieldmap.c (tileset loading), the map header data
- **Sprite missing**: object_event_graphics_portable_assets.h, event_object_movement.c, firered_video.c (OAM rendering)
- **Battle issues**: src_transformed/battle_*.c files, expect cascading bugs across multiple known families

## Game boot sequence in portable mode
Understanding the progression helps locate stalls:

    1. ROM load / init
    2. Intro sequence (copyright screen → Game Freak logo → fade)
       - Copyright assets load (INCBIN gfx/map/pal)
       - BeginNormalPaletteFade from white (~16 frames)
       - Scene transitions through callback2 chain
    3. Title screen (Charizard + logo visible ~frame 1800)
    4. Main menu → New Game / Continue
    5. New Game init:
       - Quest log, save block, Pokemon storage clear
       - ResetPokemonStorageSystem (box names, wallpapers)
    6. Overworld load:
       - LoadMapFromWarp (transition script, map data, renewables)
       - LoadMapInStepsLocal cases 0-11 (tileset copy, tilemap, BG setup)
       - FreeTempTileDataBuffersIfPossible (waits for pending DMA)
    7. Bedroom scene:
       - Opening dialogue (bedroom script, ~frames 3000-7000)
       - Player gains free movement (~frame 7000)
    8. Navigate house:
       - Stair warp to 1F (directional: requires holding Left on stair tile)
       - Door warp to Pallet Town (requires stepping onto left door tile at x=4,y=8;
         right tile x=5,y=8 has metatile behavior 0x0000 and will NOT trigger warp)
    9. Outdoor Pallet Town → Oak cutscene → Lab → Starter → Rival battle

## GBA palette pipeline (three stages)
When palette appears wrong, dump all three stages to find the break:

    gPlttBufferUnfaded[]  ← LoadPalette() writes raw palette here
    gPlttBufferFaded[]    ← UpdatePaletteFade() writes blended values here
    hardware PLTT (0x05000000) ← TransferPlttBuffer/DMA copies faded buffer here
                                 renderer reads from this

## Rule files discovered
- Root `AGENTS.md` existed and has been rewritten with repo-specific guidance.
- No `.cursor/rules/` directory was found.
- No `.cursorrules` file was found.
- No `.github/copilot-instructions.md` file was found.
- Do not assume hidden agent rules beyond this file unless the user points to them.

## Agent workflow defaults
- Do not call helper/sub-agent tools for this repo unless the user explicitly re-enables them.
- Prefer direct inspection with local tools for repo search, include tracing, and file discovery.
- Summarize large linker or build logs before deeper reasoning.
- Group unresolved failures by family: event scripts, battle scripts, text data, linker globals, BIOS shims, Godot host symbols, pointer-width issues, DMA hardware addressing, INCBIN asset ownership.
- Do not repeatedly re-diagnose a family that is already understood unless build behavior changed.

-## Common tool habits
- For local inspection, prefer `Read` for exact files, `Grep` for content search, and `Glob` for file discovery instead of shelling out.
- Use `Bash` for real builds and occasional symbol/archive inspection.
- Use `apply_patch` for most hand edits; keep patches small and file-local unless fixing a known bug family across the tree.
- Do not run headless Godot smoke tests; rely on builds, user-driven manual verification, and targeted logs/screenshots instead.
- When verifying progress, check both the runtime log and the newest snapshot images; screenshots often reveal progress past what the coarse trace shows.
- If a build or run fails with a known-family blocker, apply the biggest correct category fix rather than a one-off patch.

## Avoid
- Broad fake stubs that bypass real gameplay logic
- Catch-all duplicate-definition files
- Speculative rewrites — diagnose first, then fix with evidence
- Mass formatting passes across decomp or generated code
- Reordering fragile include stacks without a concrete reason
- Editing vendored `godot-cpp/` unless the task explicitly requires it
- Fixing one instance of a known pattern when you could fix the entire category
- Adding per-frame diagnostic prints without frame-gating (causes timeouts)
- Bypassing game state machines (e.g. faking map-load completion, skipping FreeTempTileDataBuffersIfPossible)
- Spending diagnosis rounds on problems that clearly match a known bug family — just fix them
- Re-diagnosing a bug family from scratch when the fix pattern is documented above

## Build commands
### Root ROM builds
- Default build: `make`
- Parallel CI-style build with symbols: `make -j4 all syms`
- Unix parallel variant: `make -j"$(nproc)" all syms`
- FireRed rev1: `make firered_rev1`
- LeafGreen: `make leafgreen`
- LeafGreen rev1: `make leafgreen_rev1`
- Compare against original ROM: `make compare`
- Extra compare variants mentioned in docs: `make compare_leafgreen`, `make compare_firered_rev1`, `make compare_leafgreen_rev1`
- Modern compiler path: `make modern`
- Symbol file only: `make syms`
- Regenerate generated outputs: `make generated`
- Full cleanup: `make clean`
- Rebuild helper tools after terminal/environment changes: `make clean-tools`
### GDExtension builds
- Working directory: `gdextension/`
- Default/debug build: `scons`
- Verbose build (to expose silent errors): `scons -Q` or manually extract and run the gcc command from `scons -n`
- Release build: `scons target=release`
- Library target: `scons library`
- Config generation: `scons config`
- Install into demo/project output: `scons install`
- Build everything: `scons all`
- Clean rebuild: `scons -c && scons -Q`
### Smoke test (automated regression)
- Scene: `res://scenes/runtime_smoke_test_auto.tscn`
- Do not use headless runs as a default verification path for this repo.
- Snapshot artifacts are written to `build/runtime_smoke_frame_<N>.png` and `.txt`
- The smoke test script is `scripts/runtime_smoke_test_auto.gd`
- Runtime log goes to `C:\Users\thele\AppData\Local\Temp\pokefirered_gdextension.log`
- Uses frames_per_tick=20 for speedup; snapshots only on specified frame numbers
### Manual play
- Scene: `res://scenes/runtime_smoke_test.tscn`
- Run: launch Godot normally (not headless) with the project
- Keyboard: arrows=dpad, Z=A, X=B, Enter=Start, Backspace=Select, A=L, S=R
- F12 saves a screenshot to build/
- Frame counter in corner label
### Vendored `godot-cpp` builds and tests
- Only use these when work actually touches `godot-cpp/`.
- Build bindings: `scons target=debug platform=windows` from `godot-cpp/`
- Run integration suite: `bash test/run-tests.sh` from `godot-cpp/`
- Override Godot executable if needed: `GODOT=/path/to/godot bash test/run-tests.sh`

## Lint and formatting commands
- There is no root repo-wide lint target for the main FireRed/decomp tree.
- Root formatting is convention-based; avoid introducing new formatters casually.
- `godot-cpp/` uses pre-commit for lint/format checks.
- If you modify `godot-cpp/`, run `pre-commit run --all-files` in `godot-cpp/` when available.
- Relevant vendored checks in `godot-cpp/`: `clang-format`, `ruff --fix`, `ruff-format`, `mypy`, `codespell`, `gersemi`, plus local header/file-format scripts.

## Test guidance
- The main first-party verification is a successful build.
- For ROM-equivalence-sensitive work, prefer `make compare`.
- For build-system or symbol work, prefer `make syms` or `make -j4 all syms`.
- For generated map/data changes, run `make generated` before the main build.
- For `pokefirered_core/` changes, run the root build and any affected `gdextension/` build.
- For `gdextension/` changes, run at least `scons` in `gdextension/`.
- For runtime behavior changes, prefer manual repro plus targeted logs/screenshots; do not run headless smoke tests unless the user explicitly asks.
- For `godot-cpp/` changes, run `bash test/run-tests.sh` in `godot-cpp/`.

## Single-test guidance
- No confirmed first-party single-test command exists for the root FireRed build.
- No confirmed single-test command exists for `gdextension/`; verification is build-based.
- `godot-cpp/test/run-tests.sh` runs the full Godot integration test project and does not expose a per-test selector.
- If a user asks for a single test, do not invent one; say none was found and offer the smallest real verification command instead.

## Build expectations by change type
- Gameplay/decomp C changes: run `make`; add `make compare` when byte identity matters.
- Build-system or linker changes: run `make syms` or `make -j4 all syms`.
- Portable runtime changes in `pokefirered_core/`: run the root build and the affected `gdextension/` build, then hand off for manual verification.
- Godot bridge changes in `gdextension/`: run `scons` in `gdextension/`, then hand off for manual verification.
- Vendored `godot-cpp/` changes: run its local tests and formatting checks inside that subtree.
- DMA / renderer / map-load changes: build with `scons`, then use manual repro plus runtime log/screenshot inspection.

## Generated and fragile files
- Assume files produced by `make generated` are generated outputs unless the source-of-truth is clearly elsewhere.
- Prefer editing source JSON, PNG, PAL, or script inputs rather than generated `.inc` outputs.
- Be careful with assembly include chains and macro-heavy headers; small reorderings can break builds.
- Do not replace real data paths with placeholders just to get a compile to pass.

## Style guide: general
- Follow the existing file's style before applying generic rules.
- Keep diffs small and local.
- Use ASCII by default; keep UTF-8 only where already required.
- Preserve existing naming ecosystems instead of normalizing the monorepo.
- Prefer explicit, boring code over clever abstractions.
- Add comments only when the logic is genuinely non-obvious.

## Style guide: includes and imports
- In C++, include the matching header first, then nearby project headers, then external/library headers.
- In portable C, preserve established include order unless a bug requires changing it.
- Do not reorder engine headers casually; macro definitions and typedef availability are fragile.
- Keep standard-library includes explicit instead of relying on transitive includes.
- In Python, group imports as stdlib first, then local imports; keep them sorted.

## Style guide: formatting
- Root `.editorconfig` only enforces UTF-8, so follow the local file.
- C, C++, Python, and most GDScript here generally use 4-space indentation.
- Most C and C++ files use attached/K&R braces.
- Avoid broad reflowing of comments, arrays, or alignment in untouched code.
- `godot-cpp/` is different: default indentation is tabs, Python/SConstruct use spaces, YAML uses 2 spaces, and line length is 120.

## Style guide: types
- In engine/decomp code, prefer existing typedefs such as `u8`, `u16`, `s32`, and `bool8` when the file already uses them.
- In portable runtime code, follow local style: `uint8_t`, `size_t`, `int`, and other stdint/stddef types are common.
- In C++, use `bool`, fixed-width integers where appropriate, `size_t`, `nullptr`, and Godot types such as `String` or `Ref<T>` as established nearby.
- In GDScript, use typed vars, typed signals, and typed returns when the surrounding file already does so.
- In Python tooling, add type hints for new helpers when practical.

## Style guide: naming
- C functions and locals are typically `snake_case`.
- C globals often follow decomp conventions like `gSomething`; file-local statics often use `sSomething`.
- Macros and compile-time constants are uppercase with underscores.
- C++ class names use `PascalCase`; Godot-exposed methods commonly use `snake_case`.
- GDScript uses `class_name PascalCase`, `snake_case` methods, and leading underscores for private state.
- Python modules, functions, and locals should be `snake_case`.

## Style guide: error handling
- Prefer early guard clauses.
- In C and C++, return `0`/`1`, `false`/`true`, or `NULL`/pointer results according to the local file's convention.
- Keep cleanup paths symmetrical; partial initialization should still shut down safely.
- Do not throw exceptions across the C/Godot boundary.
- In GDScript, use `push_error` or `push_warning` when the file already follows that pattern.
- In Python scripts, fail with clear messages and deterministic exit behavior.

## Style guide: control flow and structure
- Prefer straightforward procedural code over new abstraction layers.
- Match existing platform-split patterns such as `#ifdef PORTABLE` branches instead of hiding them behind speculative wrappers.
- Keep host-shell code thin; do not move engine logic into Godot unless the task explicitly requires it.
- Avoid introducing new global state when an existing runtime struct or subsystem already owns that state.

## Editing checklist
- Confirm whether the target file is first-party or vendored.
- Check whether the change affects ROM identity, generated outputs, or host integration.
- Check the bug against known families before starting diagnosis — if it matches, skip straight to the category fix.
- Make the smallest viable patch for diagnosis; make the broadest viable patch for fixes of known patterns.
- Run the narrowest real verification command that matches the edited area.
- For runtime changes, always build AND run the smoke test — a clean build does not mean the game works.
- Report clearly when no single-test command exists instead of fabricating one.

## Good defaults for agents here
- Preserve existing conventions per subtree instead of forcing one style across the whole repo.
- Prefer source-of-truth edits over generated-file edits.
- For linker failures, identify the owning object family before proposing edits.
- For large logs, compress findings before deeper reasoning.
- For uncertain generated assets or asm wrappers, trace the production path before editing.
- When a fix matches a known bug family pattern, apply it to every instance in the codebase, not just the triggering one.
- When adding portable stubs, stub the complete subsystem interface, not just the crashing function.
- After fixing a bug, consider what the NEXT 2-3 bugs in the progression will be and whether they share the same root cause.
- When receiving a batched bug report, classify each bug against known families first, fix all Tier 1 bugs in one pass, then address any that need diagnosis.
