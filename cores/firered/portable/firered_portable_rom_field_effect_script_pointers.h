#ifndef GUARD_FIRERED_PORTABLE_ROM_FIELD_EFFECT_SCRIPT_POINTERS_H
#define GUARD_FIRERED_PORTABLE_ROM_FIELD_EFFECT_SCRIPT_POINTERS_H

#include "global.h"

/*
 * Optional ROM-backed **`gFieldEffectScriptPointers`-shaped** table (70 LE u32s, FLDEFF_* order).
 * Does not replace `portable_script_pointer_resolver` tokens inside script bytecode.
 */

const u8 *firered_portable_rom_field_effect_script_ptr(u8 fldeff);

#endif /* GUARD_FIRERED_PORTABLE_ROM_FIELD_EFFECT_SCRIPT_POINTERS_H */
