#ifndef FIRERED_PORTABLE_GLOBAL_SHADOW_H
#define FIRERED_PORTABLE_GLOBAL_SHADOW_H

#include <stdint.h>

#include_next "global.h"

const void *firered_portable_resolve_script_ptr(uint32_t value);
uint32_t firered_portable_ptr_to_save_u32(const void *ptr);
uint32_t firered_portable_map_object_event_script_ptr_to_save_u32(const void *ptr);

#undef T1_READ_PTR
#undef T2_READ_PTR

#define T1_READ_PTR(ptr) ((u8 *)firered_portable_resolve_script_ptr(T1_READ_32(ptr)))
#define T2_READ_PTR(ptr) ((void *)firered_portable_resolve_script_ptr(T2_READ_32(ptr)))

#endif
