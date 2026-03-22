# ROM-backed `gStdScripts` table (portable)

Vanilla layout: **9** consecutive `u32` little-endian GBA pointers in ROM (`data/event_scripts.s`:
`gStdScripts` → `Std_ObtainItem`, `Std_FindItem`, … `Std_ReceivedItem`).

**PORTABLE** `callstd` / `gotostd` (and `_if` variants) call `firered_portable_rom_std_script_ptr(std_idx)` first.

## Overrides

1. **Environment:** `FIRERED_ROM_STD_SCRIPTS_TABLE_OFF` — hex **file offset** in the mapped ROM image to the start of the pointer table. Table length must be `4 * (gStdScriptsEnd - gStdScripts)` (same entry count as the build’s compiled table).

2. **Built-in profile:** optional rows in `firered_portable_rom_std_scripts_table.c` (CRC + `profile_id` + `table_file_off`), same matching style as other profile tables. Ships with a non-matching sentinel only.

If no override applies or a slot is `0` / out of ROM range, the runtime uses the compiled `gStdScripts[std_idx]` pointer (vanilla behavior).

## Testing (PowerShell example)

1. Find the **file offset** of `gStdScripts` in your `.gba` (same order as vanilla: 9× `u32` LE script pointers). Tools: `grep`/hex editor, or a symbol map from your build.
2. Before launching the SDL binary:

```powershell
$env:FIRERED_ROM_STD_SCRIPTS_TABLE_OFF = "0x<hex_offset>"
.\build\decomp_engine_sdl.exe
```

3. If the offset is wrong, OOB, or a slot is `0` / points outside loaded ROM, that index **falls back** to compiled `gStdScripts` (safe, vanilla-like).

## Follow-ups

- **`gSpecialVars`** — same env/profile pattern for 21× EWRAM `u16*` pointers (`docs/portable_rom_special_vars_table.md`).
- Other **small fixed-count** ROM tables (`gSpecials` is code pointers — different shape). Large families (event scripts) need token resolver work instead.
