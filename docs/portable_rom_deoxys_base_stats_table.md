# ROM-backed Deoxys base stats (`sDeoxysBaseStats`, PORTABLE)

## Purpose

**`GetDeoxysStat`** uses **`sDeoxysBaseStats[]`**: **6** base stat values for **Attack-forme Deoxys** on **FIRERED** builds (`STAT_HP` … `STAT_SPDEF`, same order as `NUM_STATS` in `include/constants/pokemon.h`).

Optional ROM load lets hacks that **retune Deoxys** (or relocate this rodata) match the cartridge. **SDL portable targets FIRERED**; LeafGreen’s Defense forme array is **not** the active compiled path here.

## Layout

- **Count:** **`FIRERED_ROM_DEOXYS_BASE_STATS_TABLE_U16_COUNT`** = **`NUM_STATS`** (6).
- **Encoding:** little-endian **`u16`** per stat, indices **`STAT_HP`** (0) through **`STAT_SPDEF`** (5).

## Configuration

- **`FIRERED_ROM_DEOXYS_BASE_STATS_TABLE_OFF`** — hex file offset into mapped ROM.
- **Profile rows** in `firered_portable_rom_deoxys_base_stats_table.c`.

## Behaviour

Unconfigured or OOB → **compiled** `sDeoxysBaseStats`. **`GetDeoxysStat`** still requires **non-link battle**, **SPECIES_DEOXYS**, etc.

## Related

- `cores/firered/portable/firered_portable_rom_deoxys_base_stats_table.{h,c}`
