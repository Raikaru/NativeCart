# Aligning this repo with **pret/pokefirered** (practical how-to)

**Role:** Step-by-step **upstream alignment** — remotes, **jsonproc** regeneration, **Makefile** / include glue, and **fork-only** toolchain notes (**agbcc**, daycare split).  
**Not here:** Drift **policy**, **pinned pret baseline (§0)**, **lane state / stopping point**, **metrics policy**, or **observed `compare_*` / ROM diagnosis** — those live in **[`upstream_reference_lane.md`](upstream_reference_lane.md)** (**§ Lane state**, **§ Metrics policy**, **§0.1**).

This tree is a **portable fork** (engine/SDL + ROM-backed data). For **vanilla** GBA fidelity, treat **fetched** **pret/pokefirered** as the source of truth for **shared data files** and **core decomp sources** you intend to match. A working **`make compare_*`** round-trip is a **separate** goal; this doc does **not** claim it is already satisfied on fork **`main`**. The lane doc records that the **pret-shaped bounded source `.text` queue** is **closed**; further compare work needs a **new explicit charter**, not routine queue extension.

**Recorded pret baseline for diffs:** **`pret/master`** @ **`9bbcab4b06ba87c3de796325461cb45a8803b9d9`** (author date ISO **2026-03-23T08:55:42-04:00**); full table and refresh policy in the lane doc **§0**.

## 1. Add pret as a remote and sync (recommended)

```bash
git remote add pret https://github.com/pret/pokefirered.git
git fetch pret
# Inspect differences, then either:
git merge pret/master   # or rebase onto pret/master
```

Resolve conflicts preferring **pret** for:

- `src/data/*.h` produced by **jsonproc** (`items.h`, `wild_encounters.h`, `src/data/region_map/*.h`, …)
- Any `src/*.c` / `include/*.h` that pret still owns for a matching ROM

Keep your **portable** layers (`engine/`, `cores/firered/`, `PORTABLE` code paths) in separate commits where possible.

## 2. Do **not** replace jsonproc outputs with portable includes

Upstream generates headers from JSON, e.g.:

- `src/data/items.h` ← `items.json` + `items.json.txt`
- `src/data/wild_encounters.h` ← `wild_encounters.json` + `wild_encounters.json.txt`
- `src/data/region_map/region_map_entries.h` / `region_map_entry_strings.h` ← `region_map_sections.json` + templates

If those files are overwritten with `#include "../../pokefirered_core/generated/..."`, **agbcc** will compile empty or wrong objects and the link step will show massive `undefined reference` errors (`CopyItemName`, wild encounter helpers, etc.).

Regenerate after pulling pret or fixing JSON:

```bash
make src/data/items.h src/data/wild_encounters.h \
  src/data/region_map/region_map_entries.h \
  src/data/region_map/region_map_entry_strings.h
# or: make generated
```

## 3. Include path for `portable/*.h` (fork glue)

Several `src/*.c` files use `#include "portable/firered_portable_*.h"`, which live under `cores/firered/portable/`. The **Makefile** adds `-iquote cores/firered` so that `portable/...` resolves like pret’s layout.

## 4. Upstream C conventions this fork had drifted from

These were restored or documented in-tree:

- **`struct Sprite`** uses the pret field name **`template`**, not `spriteTemplate` (agbcc + most `.c` files expect `.template`).
- **`include/malloc.h`** — pret-style heap API (`Alloc` / `AllocZeroed` / `Free`), not the host libc `malloc.h`.
- **`_Static_assert`** in **agbcc** builds — use **`STATIC_ASSERT`** from `global.h` in `src/*.c` where the old compiler chokes.
- **`InitScriptContext`** signature must match **`include/script.h`** (`const void *` + casts inside for `PORTABLE` vs `ScrCmdFunc *`).
- **`BattleStringExpandPlaceholdersToDisplayedString`** return type **`void`**, matching **`battle_message.h`**.
- **`StorePointerInVars` / `LoadPointerFromVars`** prototypes in **`include/battle_anim.h`** (definitions stay in `battle_anim_mons.c`).

## 5. **agbcc** `stor-layout.c` ICE on `daycare.c` (mitigated in-tree)

**agbcc** can ICE on a very large **`src/daycare.c`** translation unit (`stor-layout.c:203`). This fork splits related logic into smaller TUs (same symbols at link time):

- **`src/daycare_egg_hatch.c`** — egg hatch scene (graphics + `EggHatch` / CB2 / sprite callbacks).
- **`src/daycare_egg_moves.c`** — `Daycare_BuildEggMoveset` + egg-move table scan (`gEggMoves` / portable table).
- **`src/daycare_egg_species.c`** — `Daycare_GetEggSpecies` (same logic as pret `GetEggSpecies`). **Thumb `agbcc` ICEs** on this TU (`stor-layout.c:203`); **merging it back into the trimmed `daycare.c` also ICEs**. **`MODERN=0`** builds this object with **`agbcc_arm`** (same pattern as **`librfu_intr.o`**). That is a **codegen divergence risk** for **`make compare_*`**: if the SHA still mismatches, diff **`Daycare_GetEggSpecies`** / `.text` placement vs a pret build.

**`src/daycare.c`** includes **`daycare.h`** so calls to **`Daycare_*`** helpers are declared (avoids implicit-declaration **`Werror`** failures). **`ld_script.ld`** and **`sym_ewram.txt`** / **`sym_bss.txt`** list the extra objects next to **`daycare.o`**.

Other mitigations if you still hit toolchain issues:

- Build from **MSYS2 MinGW64** / **Git Bash** with the same flow pret documents.
- Try **`MODERN=1`** if your setup supports **`arm-none-eabi-gcc`** per Makefile.

## 6. After a successful link

- Confirm **`pokefirered.elf`** exists next to the repo root before **`gbafix`** runs (Windows path issues show up as `Error opening input file!` when the ELF step failed).
