# ROM-backed species info (`gSpeciesInfo` / `struct SpeciesInfo`, PORTABLE)

## Purpose

**`gSpeciesInfo[]`** holds per-species **base stats, types, EV yields, items, growth rate, abilities**, etc. (`struct SpeciesInfo` in `include/pokemon.h`). This is the natural **first large structured gameplay table** after medium flat slices (experience, type chart): hacks frequently **retune stats, typings, held items, or growth**, and portable SDL should be able to mirror the **loaded ROM** when the layout matches the engine build.

## Layout contract (wire format)

- **Rows:** **`NUM_SPECIES`** (412 on vanilla FireRed), index **`0 .. NUM_SPECIES - 1`** aligned with **`SPECIES_*`** (same order as compiled `gSpeciesInfo_Compiled[]`).
- **Row size:** **`26`** bytes (`FIRERED_SPECIES_INFO_WIRE_BYTES`), matching **`sizeof(struct SpeciesInfo)`** on the **portable (GCC) build**.
- **Encoding (little-endian where applicable):**

| Offset | Size | Content |
|--------|------|---------|
| 0 | 6×`u8` | `baseHP` … `baseSpDefense` |
| 6 | 2×`u8` | `types[0]`, `types[1]` |
| 8 | `u8` | `catchRate` |
| 9 | `u8` | `expYield` |
| 10–11 | `u16` LE | Packed EV yields: 6×2-bit fields in order **HP, Atk, Def, Spd, SpAtk, SpDef** (GCC little-endian layout) |
| 12–13 | `u16` LE | `itemCommon` |
| 14–15 | `u16` LE | `itemRare` |
| 16 | `u8` | `genderRatio` |
| 17 | `u8` | `eggCycles` |
| 18 | `u8` | `friendship` |
| 19 | `u8` | `growthRate` |
| 20–21 | 2×`u8` | `eggGroups` |
| 22–23 | 2×`u8` | `abilities` |
| 24 | `u8` | `safariZoneFleeRate` |
| 25 | `u8` | `bodyColor` (low 7 bits) + `noFlip` (bit 7) |

- **Total blob size:** **`NUM_SPECIES * 26`** bytes (e.g. **10712** for `NUM_SPECIES == 412`).

## Validation (fail-safe)

1. **`sizeof(struct SpeciesInfo) == 26`** on the portable target.
2. **Unpack self-test:** wire-decode **`gSpeciesInfo_Compiled[SPECIES_BULBASAUR]`** must **memcmp-equal** the compiled struct (guarantees bit packing matches this decoder).
3. **Per-row structural checks** (after decode): `types[0/1] < NUMBER_OF_MON_TYPES`, `growthRate <= GROWTH_SLOW`.

On any failure → ROM cache **not** activated; **`gSpeciesInfoActive`** stays **NULL** and the **`gSpeciesInfo`** macro falls back to **`gSpeciesInfo_Compiled`**.

## Configuration

- **`FIRERED_ROM_SPECIES_INFO_TABLE_OFF`** — hex file offset into mapped ROM (`firered_portable_rom_u32_table_resolve_base_off`).
- **Profile rows** in `firered_portable_rom_species_info_table.c`.

## Runtime wiring (PORTABLE)

- Compiled table symbol: **`gSpeciesInfo_Compiled[]`** (see `src/data/pokemon/species_info.h`).
- **`gSpeciesInfoActive`** points at the ROM cache when active, else **NULL**.
- **`#define gSpeciesInfo ((gSpeciesInfoActive) != NULL ? (gSpeciesInfoActive) : (gSpeciesInfo_Compiled))`** in `include/pokemon.h` — existing **`gSpeciesInfo[...]`** call sites stay unchanged.

Non-**PORTABLE** builds keep **`const struct SpeciesInfo gSpeciesInfo[]`** unchanged.

## Limitations

- **`NUM_SPECIES`** and **`struct SpeciesInfo`** layout are still **build-time**; this slice does **not** support expanded species counts or different struct layouts without **rebuilding** the engine and revising the contract.
- ROM must match **this** wire format; arbitrary **agbcc** vs **GCC** struct packing differences vs the decoder will fail the self-test and force compiled fallback.

## Related

- `cores/firered/portable/firered_portable_rom_species_info_table.{h,c}`
- `include/pokemon.h`, `src/data/pokemon/species_info.h`
