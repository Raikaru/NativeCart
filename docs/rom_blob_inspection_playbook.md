# ROM blob inspection playbook (portable migrations)

This repo maps many optional portable features to **file offsets** into the loaded ROM image (`FIRERED_ROM_*` env vars, documented per table in `docs/portable_rom_*.md` and summarized in `docs/rom_backed_runtime.md`). Before you commit offsets for a new hack or rebased build, it helps to **slice the same bytes the runtime will read** and check length, alignment, and simple patterns.

## Tool: `tools/rom_blob_inspect.py`

Read-only helper: given a ROM path, **file offset**, and **length**, it prints slice bounds, optional **CRC32** of the slice, optional **hex dump**, and **little-endian `u16` / `u32` summaries**. Optional checks fail the process (exit `1`) so you can use it in scripts:

| Flag | Purpose |
|------|--------|
| `--struct-stride BYTES` | Require `length % BYTES == 0` (e.g. `26` for species info rows, `4` for `u32` pointer tables) |
| `--expect-len BYTES` | Require exact slice length |
| `--gba-u32-pointers` | Classify each `u32` as zero, in-ROM `0x08……` pointer, GBA ROM window but outside file, or other (works with `--no-stats` to print only this line) |
| `--u8-cols STRIDE` | Treat the slice as **fixed-width `u8` rows** of `STRIDE` bytes; print **per-column** min / max / zero counts (e.g. **`9`** for `struct BattleMove` wire rows, **`3`** for type-chart-style triplets) |
| `--validate-egg-moves-u16` | Parse a FireRed-style **`gEggMoves`** `u16` stream (species `+ 20000` headers, move IDs, trailing **`0xFFFF`**). Optional `--egg-species-offset`, `--egg-num-species`, `--egg-moves-count` for forks |

Offsets and lengths accept **`0x` hex** or decimal (underscores allowed).

**Next seams** (egg stream, learnset pointers, items / friendship): see **`docs/next_rom_structural_seam_notes.md`**.

### Examples

```text
python tools/rom_blob_inspect.py path/to/game.gba -o 0x3FAF0C -n 18 --struct-stride 2
python tools/rom_blob_inspect.py path/to/game.gba -o 0x123400 -n 0x100 --hexdump --hex-lines 8
python tools/rom_blob_inspect.py path/to/game.gba -o 0x400000 -n 0x200 --gba-u32-pointers --no-stats
python tools/rom_blob_inspect.py path/to/game.gba -o 0x<egg_off> -n 0x<egg_len> --validate-egg-moves-u16 --no-stats
python tools/rom_blob_inspect.py path/to/game.gba -o 0x<moves_off> -n 3195 --u8-cols 9 --struct-stride 9 --no-stats
```

Cross-check **expected byte counts** against the doc for that table (e.g. HM moves: **9 × `u16`** + `0xFFFF` sentinel ⇒ **18 bytes**). For **`u32[]` overlay** tables, use `--gba-u32-pointers` to spot stray words and zero slots the overlay treats as “use compiled.”

## Workflow notes

1. **Identify the slice** — Use your map, sym file, or upstream decomp label; convert to **file offset** (same convention as `FIRERED_ROM_*`: bytes from the start of the mapped ROM buffer, not GBA bus addresses unless you have already converted).
2. **Match documented layout** — `docs/rom_backed_runtime.md` links each **`FIRERED_ROM_*`** to a focused doc with **width × count** and validation rules the C code applies.
3. **Optional CRC** — `--crc32` on the slice is a quick fingerprint when comparing two ROMs after a small edit; it is **not** a substitute for structural checks.

This playbook is intentionally narrow: it does not duplicate the architecture narrative in `docs/rom_compat_architecture.md`.

## Layout companion bundle placement (optional)

Before choosing file offset **`P`** for **`FIRERED_ROM_MAP_LAYOUT_*_OFF`** on a real **`.gba`**, use **`tools/portable_generators/layout_rom_companion_audit_rom_placement.py`** for bounds + slice statistics, and **`rom_blob_inspect.py`** on the same range for a visual check. See **`docs/portable_rom_map_layout_metatiles.md`** (“Choosing `P` on a real `.gba`”).
