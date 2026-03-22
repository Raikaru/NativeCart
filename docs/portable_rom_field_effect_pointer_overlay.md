# ROM-backed field-effect pointer overlay (`0x82000000` resolver)

## Purpose

Portable bytecode can encode references as **`0x82000000 + i`** → **`gFireredPortableFieldEffectScriptPtrs[i]`** (`field_effect_scripts_portable_data.c`). This overlay lets hacks supply a parallel **`u32[]`** of **GBA ROM addresses** so **`firered_portable_resolve_script_ptr`** follows the **loaded cartridge** for relocated **ROM-resident** data (e.g. palettes) while keeping **compiled fallback** for everything else.

## Not the same table as `FieldEffectStart`

| Mechanism | Env | Entry count | Order |
|-----------|-----|-------------|--------|
| **`FieldEffectStart(fldeff)`** | **`FIRERED_ROM_FIELD_EFFECT_SCRIPT_PTRS_OFF`** | **70** | **`FLDEFF_*`** enum (`gFieldEffectScriptPointers`) |
| **This resolver overlay** | **`FIRERED_ROM_FIELD_EFFECT_PTRS_OFF`** | **`gFireredPortableFieldEffectScriptPtrCount`** | **Portable token** order (`generate_portable_event_script_data.py` on `field_effect_scripts.s`) |

Do not point both env vars at the same offset unless a tool explicitly builds **two** aligned tables.

## Interleaved semantics (native vs ROM data)

`gFireredPortableFieldEffectScriptPtrs` mixes **host `FldEff_*` natives** (from `callnative` in scripts), **`gSpritePalette_*` / `gFldEffPalette_*`** (ROM palette data), and similar. There is **no** clean index prefix that is “always ROM.”

**Rule:** the overlay **only accepts** **`0x08000000`–`0x09FFFFFF`** inside the **loaded** ROM image. Anything else (including **0**) → **compiled** `gFireredPortableFieldEffectScriptPtrs[i]`.

**Native handler slots:** keep the parallel ROM word **`0`** so the resolver keeps the **host** `FldEff_*` pointer. Putting a **ROM ARM** address there is unsafe (portable expects a host function for those entries).

## Configuration

1. **`FIRERED_ROM_FIELD_EFFECT_PTRS_OFF`** — hex file offset to **`gFireredPortableFieldEffectScriptPtrCount`** little-endian `u32` values, **index-aligned** with `gFireredPortableFieldEffectScriptPtrs`.
2. **Profile rows** (optional): `s_field_effect_ptr_overlay_profile_rows` in `firered_portable_rom_field_effect_pointer_overlay.c`.

## Related

- `cores/firered/portable/firered_portable_rom_field_effect_pointer_overlay.{h,c}`
- `cores/firered/portable/portable_script_pointer_resolver.c`
- `docs/portable_rom_field_effect_script_pointers.md` — **`FieldEffectStart`** / **`FLDEFF_*`** table
- `docs/rom_backed_runtime.md` §6d, §6k
