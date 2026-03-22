# ROM-backed TM/HM learnsets (`sTMHMLearnsets`, PORTABLE)

## Purpose

**`sTMHMLearnsets`** (compiled as **`sTMHMLearnsets_Compiled`** on PORTABLE) is the per-species **64-bit TM/HM compatibility mask** split into **two little-endian `u32`** words. **`CanMonLearnTMHM`** is the sole consumer — a natural, bounded follow-up to **`gSpeciesInfo`**: hacks often change which machines each species can learn.

## Layout contract

- **Rows:** **`NUM_SPECIES`**, index-aligned with **`SPECIES_*`** (same as compiled table).
- **Row width:** **2 × `u32`** = **8** bytes (low **32** bits of the 64-bit mask, then high **32** bits — same as `TMHM_LEARNSET` in `tmhm_learnsets.h`).
- **Total blob:** **`NUM_SPECIES * 8`** bytes (e.g. **3296** when `NUM_SPECIES == 412`).
- **Endianness:** **Little-endian `u32`** words (matches GBA and the portable host).

## Validation (fail-safe)

1. **ROM bounds** and minimum ROM size (shared `u32` table helper).
2. **`SPECIES_NONE`** row must be **(0, 0)** — matches vanilla and catches many garbage offsets.

There is **no** row-by-row equality check against compiled data so **learnset hacks** can still activate when the blob is otherwise valid.

## Configuration

- **`FIRERED_ROM_TMHM_LEARNSETS_TABLE_OFF`** — file offset into mapped ROM (`firered_portable_rom_u32_table_resolve_base_off`).
- **Profile rows** in `firered_portable_rom_tmhm_learnsets_table.c`.

## Runtime wiring (PORTABLE)

- **`gTMHMLearnsetsActive`** points at the ROM cache when active, else **NULL**.
- After including **`data/pokemon/tmhm_learnsets.h`**, `pokemon.c` defines  
  **`#define sTMHMLearnsets ((gTMHMLearnsetsActive) != NULL ? (gTMHMLearnsetsActive) : (sTMHMLearnsets_Compiled))`**  
  so **`CanMonLearnTMHM`** stays unchanged.

Non-**PORTABLE** builds keep **`static const u32 sTMHMLearnsets[][2]`**.

## Limitations

- **`NUM_SPECIES`** is fixed at build time; expanded-species hacks require an engine rebuild and a matching blob length.
- Wrong offset with **`SPECIES_NONE` zero** in ROM could still validate — treat env offsets as trusted; profile rows recommended for production use.

## Related

- `cores/firered/portable/firered_portable_rom_tmhm_learnsets_table.{h,c}`
- `src/data/pokemon/tmhm_learnsets.h`, `src/pokemon.c` / `src_transformed/pokemon.c`
- `include/pokemon.h` — **`extern`** **`gTMHMLearnsetsActive`**
