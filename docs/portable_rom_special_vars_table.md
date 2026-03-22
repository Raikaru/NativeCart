# ROM-backed `gSpecialVars` table (portable)

Vanilla: **21** consecutive `u32` little-endian words in ROM (`data/event_scripts.s`, before `gStdScripts`), each the GBA address of a `u16` special var (`gSpecialVar_0x8000` … `gSpecialVar_0x8014`).

**PORTABLE** `GetVarPointer()` uses `firered_portable_rom_special_var_ptr(slot)` when `slot = idx - SPECIAL_VARS_START` is in range.

## Validation

Resolved pointers must be **2-byte aligned** and fall in **`ENGINE_EWRAM_ADDR` … + `ENGINE_EWRAM_SIZE`** (same region as `EWRAM_DATA gSpecialVar_*`).

## Overrides

1. **Environment:** `FIRERED_ROM_SPECIAL_VARS_TABLE_OFF` — hex file offset to the start of the 21×`u32` table in the mapped ROM.
2. **Profile:** optional rows in `firered_portable_rom_special_vars_table.c` (CRC + `profile_id` + offset). Ships with a non-matching sentinel only.

If unset or invalid → compiled `gSpecialVars[slot]` (vanilla behavior).

## Testing

```powershell
$env:FIRERED_ROM_SPECIAL_VARS_TABLE_OFF = "0x<hex_offset>"
.\build\decomp_engine_sdl.exe
```

ROM words must match the **runtime EWRAM layout** (typically vanilla 0x0200… addresses when using a retail-aligned build).

## Follow-ups

Other small ROM indirection tables; **not** `gSpecials` (code pointers). Large script token families remain separate work.
