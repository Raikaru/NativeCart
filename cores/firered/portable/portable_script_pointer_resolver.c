#include "global.h"
#include "../../../engine/core/engine_internal.h"

const void *firered_portable_resolve_script_ptr(uint32_t value)
{
    if (value == 0)
        return NULL;

    return (const void *)(uintptr_t)value;
}
