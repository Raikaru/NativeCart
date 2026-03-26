# CFRU upstream reference (links and build model)

Short companion to **`docs/cfru_integration_playbook.md`**. Use this for **where CFRU lives**, **how it is produced upstream**, and **what to read first** there. This file does **not** duplicate CFRU feature lists in full—see the upstream README. It is an **orientation / source-reference** doc, **not** a claim that this repo currently runs CFRU-class hacks end-to-end.

## 1. Official repositories

| Project | URL | Role |
|---------|-----|------|
| **Complete Fire Red Upgrade (CFRU)** | [github.com/Skeli789/Complete-Fire-Red-Upgrade](https://github.com/Skeli789/Complete-Fire-Red-Upgrade) | Battle engine upgrade, scripting/save/QoL extensions, insertion toolchain |
| **Dynamic Pokémon Expansion (DPE)** | [github.com/Skeli789/Dynamic-Pokemon-Expansion](https://github.com/Skeli789/Dynamic-Pokemon-Expansion) | Species expansion; often used **with** CFRU in modern FireRed hacks |

## 2. Upstream documentation entry points

| Resource | URL / location | Notes |
|----------|----------------|--------|
| **README** | [Complete-Fire-Red-Upgrade/README.md](https://github.com/Skeli789/Complete-Fire-Red-Upgrade/blob/master/README.md) | Feature overview, **terms of use** (read before redistributing or monetizing anything derived from CFRU assets), pointer to wiki |
| **Windows install** | [Wiki — Windows Installation Instructions](https://github.com/Skeli789/Complete-Fire-Red-Upgrade/wiki/Windows-Installation-Instructions) | devkitPro / Python / build hygiene |
| **Engine scripts** | Described in CFRU wiki / README | Header-wide rebuilds (`clean.py`) on Windows vs Unix |

**Compliance:** Treat CFRU’s README as the **source of truth** for what upstream allows (including **non‑commercial** use and related restrictions). This decomp fork’s docs do **not** grant rights beyond those terms.

## 3. How CFRU is built upstream (ROM insertion)

High-level flow (from public README; **exact script names / helper layout may change upstream**):

1. **Toolchain:** devkitPro / **devkitARM** (`arm-none-eabi-*`), **Python 3.6+**.
2. **Base ROM:** Name the clean **BPRE** image **`BPRE0.gba`** in the CFRU repo root (US FireRed).
3. **Insert offset:** Configure **`OFFSET_TO_PUT`** in **`scripts/make.py`** for where the upgrade is injected.
4. **Build:** Run **`python scripts/make.py`** (or `python3` as documented).
5. **Outputs:** Typically **`test.gba`** (patched ROM) and **`offsets.ini`** (resolved addresses for the insertion).

**Why `offsets.ini` matters:** CFRU and downstream hacks often treat those resolved insertion addresses as part of the build/integration contract. For this repo, that reinforces why CFRU is an **external binary/tooling workflow reference**, not a simple “copy source files into the decomp” dependency.

CFRU is **not** a “clone and `make` like pokefirered” project: the **artifact** is a **modified ROM** plus metadata, not a standalone ELF you can run on PC. Expect this doc to require occasional refresh if upstream reorganizes scripts, wiki pages, or documented setup steps.

## 4. Configuration surface upstream

| Location | Purpose |
|----------|---------|
| **`src/config.h`** (CFRU) | Compile-time toggles (comment/uncomment options per upstream docs) |

Hack authors and CFRU forks may carry **additional** private config; this table is only the **documented** public knob.

## 5. Relationship to pret `pokefirered`

| | pret `pokefirered` | CFRU |
|--|-------------------|------|
| **Shape** | C decompilation of retail, reproducible ROM build | **Patch/insert** upgrade on top of a **stock BPRE** binary workflow |
| **Integration story** | Merge C changes, `compare_firered` | **Semantic port** or **binary pipeline**—not the same as including a `.c` “library” |

This fork’s **native portable** path aligns with **pret-shaped C** plus **`engine/`** shims; CFRU remains an **external reference** unless you explicitly merge or reimplement behavior.

## 6. Other native Gen III references (Emerald PC-port forks)

*Not CFRU-specific.* Included here as a **single link hub** next to other upstream pointers; the ROM-backed migration docs also reference this section.

**Yes — as a patterns reference, not as a merge source.** Several community efforts port **pret `pokeemerald`** to **desktop/SDL** (or similar). A commonly cited line of work is **[Sierraffinity/pokeemerald](https://github.com/Sierraffinity/pokeemerald)**, branch **`pc-port`** — treat the **exact branch name and layout** as **upstream-controlled**; verify on GitHub before deep linking to paths.

| Useful for | Less useful for |
|------------|-----------------|
| **Host main loop**, **SDL init**, **input/audio/video** split ideas | **FireRed-specific** files, **CFRU** battle semantics, **BPRE** table layouts |
| **How another team** isolated **REG_*** / VRAM fiction from game logic | **Drop-in** `.c` files: **Emerald ≠ FireRed** (maps, scripts, battle tables, constants) |
| **Prior art** when arguing about **shim boundaries** (what lives in “platform” vs “game”) | **1:1 symbol parity** with this repo’s `engine/core/` — architectures diverge on purpose |

**Caveat:** Official **[pret/pokeemerald](https://github.com/pret/pokeemerald)** `master` is the **GBA decomp**, not a PC runtime. PC-native work lives in **forks/branches**; quality, maintenance, and goals vary—**audit before adopting** a pattern wholesale.

## 7. Related docs in this repo

- **`docs/cfru_integration_playbook.md`** — blockers, compatibility layer, milestones for **this** tree  
- **`docs/portable_rom_compat.md`** — ROM fingerprint / `profile_id` (e.g. Unbound hints)  
- **`docs/rom_compat_architecture.md`** — ROM-backed table matrix and AGB-bridge notes  
