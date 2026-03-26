#ifndef GUARD_FIRERED_HEAP_H
#define GUARD_FIRERED_HEAP_H

#include "global.h"

#define HEAP_SIZE 0x1C000
/*
 * Host/engine TUs that must keep libc malloc/free (e.g. ROM copies) should
 * `#define FIRERED_HOST_LIBC_MALLOC 1` before any include that pulls this header.
 */
#ifndef FIRERED_HOST_LIBC_MALLOC
#define malloc Alloc
#define calloc(ct, sz) AllocZeroed((ct) * (sz))
#define free Free
#endif

#ifndef TRY_FREE_AND_SET_NULL
#define TRY_FREE_AND_SET_NULL(ptr) if (ptr != NULL) FREE_AND_SET_NULL(ptr)
#endif

extern u8 gHeap[];
void *Alloc(u32 size);
void *AllocZeroed(u32 size);
void Free(void *pointer);
void InitHeap(void *pointer, u32 size);

#endif // GUARD_FIRERED_HEAP_H
