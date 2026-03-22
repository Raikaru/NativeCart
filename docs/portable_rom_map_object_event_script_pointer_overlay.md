# ROM-backed map object event script pointer overlay (`0x85000000` resolver)

## Purpose

On **PORTABLE**, map `ObjectEventTemplate.script` fields are fixed up to portable tokens; resolution of **`0x85000000 + i`** uses **`gFireredPortableMapObjectEventScriptPtrs[i]`** — a **sorted list of unique** `EventScript_*` symbols from non-clone object events (`generate_portable_map_data.py`).

Hacks that **relocate** those scripts in ROM while keeping the **same logical symbol order** in the portable table can supply a parallel **`u32[]`** of GBA ROM pointers so **`firered_portable_resolve_script_ptr`** returns addresses into the **loaded cartridge** instead of the decomp snapshot.

## No ScrCmd-style prefix

Unlike **`gFireredPortableEventScriptPtrs`** (0x81), this table has **no** leading command-dispatch entries: **every** slot is an object-event script. **`FIRERED_PORTABLE_MAP_OBJECT_EVENT_SCRIPT_ROM_OVERLAY_FIRST_INDEX`** is **`0`**. If the generator ever prepends non-ROM pointers, bump that constant and document the skip here.

## Configuration

1. **`FIRERED_ROM_MAP_OBJECT_EVENT_SCRIPT_PTRS_OFF`** — hex file offset to **`gFireredPortableMapObjectEventScriptPtrCount`** little-endian `u32` values, **same index order** as `gFireredPortableMapObjectEventScriptPtrs` (alphabetical unique symbol order from the generator).
2. **Profile rows** (optional): `s_map_object_event_script_ptr_overlay_profile_rows` in `firered_portable_rom_map_object_event_script_pointer_overlay.c`.

## Semantics

- If env/profile does not resolve a base, or the read is OOB, or **`u32[i]`** is **0**, or the value is not a **ROM-region** pointer within the **loaded** image → **`gFireredPortableMapObjectEventScriptPtrs[i]`** (vanilla when unset).
- **`firered_portable_map_object_event_script_ptr_to_save_u32`** is unchanged; only **resolve** gains the overlay.

## Related

- `cores/firered/portable/firered_portable_rom_map_object_event_script_pointer_overlay.{h,c}`
- `cores/firered/portable/portable_script_pointer_resolver.c`
- `tools/portable_generators/generate_portable_map_data.py` (`collect_map_object_script_symbols`)
- `docs/rom_backed_runtime.md` §6i
