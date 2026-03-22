#ifndef GUARD_FIRERED_PORTABLE_ROM_FIELD_EFFECT_POINTER_OVERLAY_H
#define GUARD_FIRERED_PORTABLE_ROM_FIELD_EFFECT_POINTER_OVERLAY_H

#include "global.h"

/*
 * Optional ROM-backed u32[] parallel to gFireredPortableFieldEffectScriptPtrs[] for portable
 * tokens 0x82000000 + index (firered_portable_resolve_script_ptr).
 *
 * This is NOT the same table as gFieldEffectScriptPointers / FLDEFF_* order used by
 * FieldEffectStart — that uses FIRERED_ROM_FIELD_EFFECT_SCRIPT_PTRS_OFF (70 entries).
 * This overlay matches generate_portable_event_script_data token order for the field-effect
 * script pointer family (see docs/portable_rom_field_effect_pointer_overlay.md).
 *
 * The portable table mixes host-native FldEff_* handlers, ROM palette blobs (gSpritePalette_*),
 * etc. The overlay only returns a pointer when the ROM word is non-zero and in the GBA ROM
 * window (0x08000000 - 0x09FFFFFF) inside the loaded image. Use ROM word 0 for indices that
 * must keep compiled host pointers (native FldEff_*); otherwise a stray ARM address could
 * replace a host function pointer incorrectly.
 */

#define FIRERED_PORTABLE_FIELD_EFFECT_ROM_OVERLAY_FIRST_INDEX 0u

const u8 *firered_portable_rom_field_effect_pointer_overlay_ptr(u32 token_index);

#endif /* GUARD_FIRERED_PORTABLE_ROM_FIELD_EFFECT_POINTER_OVERLAY_H */
