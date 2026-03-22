# ROM-backed field effect script pointer table (PORTABLE / SDL)

## Purpose

On cartridge, `gFieldEffectScriptPointers[fldeff]` is an array of pointers into **ROM** (starts of field-effect bytecode). The portable build keeps a decomp snapshot in `field_effect_scripts_portable_data.c`.

This seam lets SDL load the **same logical table** from the **mapped ROM** when the table’s file offset is known, so hacks that **relocate** field effect scripts (but keep retail bytecode + GBA addresses inside scripts) can run without regenerating portable data.

**Resolver `0x82` tokens:** bytecode tokens **`0x82000000 + i`** still go through **`firered_portable_resolve_script_ptr`**, which can optionally use a **separate** ROM parallel table via **`FIRERED_ROM_FIELD_EFFECT_PTRS_OFF`** (portable token order, not `FLDEFF_*`). See `docs/portable_rom_field_effect_pointer_overlay.md`.

## Configuration

1. **`FIRERED_ROM_FIELD_EFFECT_SCRIPT_PTRS_OFF`** — hex file offset (via `firered_portable_rom_asset_parse_hex_env`) to the start of **70** consecutive little-endian `u32` values (same order as `gFieldEffectScriptPointers`: `FLDEFF_EXCLAMATION_MARK_ICON` … `FLDEFF_PHOTO_FLASH`).
2. **Profile rows** (optional): edit `s_field_effect_script_ptrs_profile_rows` in `firered_portable_rom_field_effect_script_pointers.c` (CRC + `profile_id` + `table_file_off`), same pattern as other ROM u32 tables.

## Semantics

- Each `u32` must be a **GBA ROM address** (`0x08……`–`0x09……`) pointing at the **first byte** of that field effect’s script; `0` is invalid (falls back to compiled `gFieldEffectScriptPointers[fldeff]`).
- Table must lie fully inside the loaded ROM image.
- **`FieldEffectStart`** tries ROM first when `PORTABLE`; on failure, uses compiled pointers (vanilla unchanged).

## Related

- `cores/firered/portable/firered_portable_rom_u32_table.{h,c}` — shared env/profile + bounds + LE read.
- `docs/rom_backed_runtime.md` §6d.
