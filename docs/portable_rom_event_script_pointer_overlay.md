# ROM-backed event script pointer overlay (`0x81000000` resolver)

## Purpose

Portable saves and script bytecode encode many references as **`0x81000000 + i`** → `gFireredPortableEventScriptPtrs[i]`. That table is a **decomp snapshot**; hacks that **relocate** map scripts, text blobs, and other event-side data in the ROM keep **retail GBA addresses** inside bytecode while portable still resolves the **old** host pointers.

This seam adds an optional **ROM `u32[]`** with the **same length and token order** as `gFireredPortableEventScriptPtrs[]`. When a base offset is configured, **`firered_portable_resolve_script_ptr`** reads **`u32[i]`** from the **loaded cartridge** for **`i ≥ FIRERED_PORTABLE_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX`**, validates it as a **ROM-region** pointer, and uses it; otherwise it keeps the compiled table entry.

**Not in scope:** `gSpecialVars` / `gStdScripts` / `gSpecials` (separate ROM tables), regenerating `event_scripts_portable_data.c`, or a full `portable_script_pointer_resolver` rewrite. Map-object **`0x85000000`** tokens have a separate overlay — `docs/portable_rom_map_object_event_script_pointer_overlay.md`.

## ScrCmd prefix (never ROM-backed)

The first **`FIRERED_PORTABLE_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX` (213)** slots mirror **`gScriptCmdTable`** (`data/script_cmd_table.inc`, opcodes **`0x00`–`0xD4`**). Those entries are **host function pointers** to **`ScrCmd_*`** implementations. The overlay **never** reads them from ROM so we never treat ARM/Thumb code addresses as script bytecode.

**If `script_cmd_table.inc` gains opcodes**, bump **`FIRERED_PORTABLE_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX`** in `firered_portable_rom_event_script_pointer_overlay.h` to match the new ScrCmd count.

## Configuration

1. **`FIRERED_ROM_EVENT_SCRIPT_PTRS_OFF`** — hex file offset to the start of **`gFireredPortableEventScriptPtrCount`** consecutive little-endian `u32` values, **index-aligned** with `gFireredPortableEventScriptPtrs` (same generator order as `generate_portable_event_script_data.py`).
2. **Profile rows** (optional): edit `s_event_script_ptr_overlay_profile_rows` in `firered_portable_rom_event_script_pointer_overlay.c` (CRC + `profile_id` + `table_file_off`), same pattern as other ROM u32 tables.

## Semantics

- For **`i < 213`**: ROM table is ignored; resolver uses **`gFireredPortableEventScriptPtrs[i]`** only.
- For **`i ≥ 213`**: if env/profile supplies a valid base and **`u32[i]`** is non-zero, lies in **`0x08000000`–`0x09FFFFFF`**, and is within the **loaded ROM** size, return that address; else fall back to **`gFireredPortableEventScriptPtrs[i]`**.
- If the ROM block is missing, OOB, or too small for the full table, **`firered_portable_rom_u32_table_read_entry`** fails → compiled fallback (vanilla unchanged when unset).

## Related

- `cores/firered/portable/firered_portable_rom_event_script_pointer_overlay.{h,c}`
- `cores/firered/portable/portable_script_pointer_resolver.c` — `FIRERED_PORTABLE_PTR_TOKEN_BASE_EVENT` branch
- `cores/firered/portable/firered_portable_rom_u32_table.{h,c}`
- `docs/rom_backed_runtime.md` §6h
