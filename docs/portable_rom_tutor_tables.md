# ROM-backed move tutor tables (`sTutorMoves` / `sTutorLearnsets`, PORTABLE)

## Purpose

FireRed’s **move tutor** UI uses:

- **`sTutorMoves[TUTOR_MOVE_COUNT]`** — **`TUTOR_MOVE_COUNT`** (15) little-endian **`u16`** move IDs (`TUTOR_MOVE_MEGA_PUNCH` … `TUTOR_MOVE_SUBSTITUTE`).
- **`sTutorLearnsets[NUM_SPECIES]`** — per-species **`u16`** bitmasks; bit **`i`** set iff species can learn tutor slot **`i`** (same encoding as **`TUTOR(MOVE_*)`** in `src/data/pokemon/tutor_learnsets.h`).

Portable SDL can mirror both from the loaded ROM when layouts match. **Starter-only tutors** (Frenzy Plant / Blast Burn / Hydro Cannon) stay **hardcoded** in `GetTutorMove` / `CanLearnTutorMove` in **`party_menu.c`** — this slice does not replace those branches.

## Layout contract (wire format)

### Tutor move list

- **Bytes:** **`TUTOR_MOVE_COUNT * 2`** (e.g. **30** when count is 15).
- **Encoding:** **`u16`** little-endian per slot, index order **`0 … TUTOR_MOVE_COUNT - 1`**.
- **Validation:** each word **`< MOVES_COUNT`**.

### Tutor learnsets

- **Bytes:** **`NUM_SPECIES * 2`** (e.g. **824** when `NUM_SPECIES == 412`).
- **Encoding:** **`u16`** LE per species, index **`0 … NUM_SPECIES - 1`** (same as **`SPECIES_*`**).
- **Validation:** **`SPECIES_NONE`** entry **0**; every word **`≤ (1 << TUTOR_MOVE_COUNT) - 1`** (only bits **`0 … TUTOR_MOVE_COUNT - 1`** may be set — **15 bits** for vanilla).

## Runtime wiring

- **Env / profile bases:**
  - **`FIRERED_ROM_TUTOR_MOVES_TABLE_OFF`**
  - **`FIRERED_ROM_TUTOR_LEARNSETS_TABLE_OFF`**
- Both must resolve, fit in ROM, **and** validate, or **neither** ROM path activates (no mixed ROM/compiled pairing).
- **Refresh:** **`firered_portable_rom_tutor_tables_refresh_after_rom_load()`** from **`engine_runtime_backend.c`** (after battle moves).
- **Access:** **`gTutorMovesActive`** / **`gTutorLearnsetsActive`** in **`party_menu.h`**; **`party_menu.c`** macros select ROM cache vs **`sTutorMoves_Compiled` / `sTutorLearnsets_Compiled`** from **`tutor_learnsets.h`**.

## Non-portable builds

Static **`sTutorMoves` / `sTutorLearnsets`** only; no ROM refresh.

## Tooling

Use **`tools/rom_blob_inspect.py`** with **`--struct-stride 2`** on each slice for quick LE **`u16`** stats; see **`docs/rom_blob_inspection_playbook.md`**.
