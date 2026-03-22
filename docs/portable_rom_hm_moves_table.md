# ROM-backed HM move list (`sHMMoves`, PORTABLE)

## Purpose

`IsHMMove2` uses **`sHMMoves[]`** in `pokemon.c`: a **`u16`** list of HM **move IDs** ending with **`0xFFFF`** (`HM_MOVES_END`). Retail FireRed lists **8** HMs then the sentinel (**9** `u16` total).

Optional ROM load lets hacks that **add, remove, or remap HMs** (while keeping the same **9-word** layout) match the cartridge without rebuilding the SDL binary.

## Layout

- **Count:** **`FIRERED_ROM_HM_MOVES_TABLE_U16_COUNT`** = **9** little-endian **`u16`** values.
- **Termination:** at least one word must equal **`0xFFFF`** within those 9 words (load fails otherwise — avoids unbounded scans).
- **Semantics:** same as compiled `sHMMoves` — entries before the first **`0xFFFF`** are HM move IDs checked by **`IsHMMove2`**.

## Configuration

- **`FIRERED_ROM_HM_MOVES_TABLE_OFF`** — hex file offset into mapped ROM.
- **Profile rows** in `firered_portable_rom_hm_moves_table.c`.

## Behaviour

Unconfigured, OOB, or missing sentinel → **compiled** `sHMMoves` (vanilla).

**Note:** Longer HM lists require a **different build constant** (`FIRERED_ROM_HM_MOVES_TABLE_U16_COUNT`); this slice is intentionally **retail-sized**.

## Related

- `cores/firered/portable/firered_portable_rom_hm_moves_table.{h,c}`
