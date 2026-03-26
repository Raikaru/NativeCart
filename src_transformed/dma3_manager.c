#include "global.h"
#include "dma3.h"

#ifdef PORTABLE
#include <stdio.h>

extern void firered_runtime_trace_external(const char *message);

#define PORTABLE_PLTT_BASE 0x05000000u
#define PORTABLE_PLTT_SIZE 0x00000400u
#define PORTABLE_VRAM_BASE 0x06000000u
#define PORTABLE_VRAM_SIZE 0x00018000u
#define PORTABLE_OAM_BASE  0x07000000u
#define PORTABLE_OAM_SIZE  0x00000400u

static void *PortableResolveDmaAddress(const void *ptr, const char **rangeName, bool8 *isHardware)
{
    uintptr_t address = (uintptr_t)ptr;

    *rangeName = "none";
    *isHardware = FALSE;

    if (address >= PORTABLE_PLTT_BASE && address < PORTABLE_PLTT_BASE + PORTABLE_PLTT_SIZE)
    {
        *rangeName = "pltt";
        *isHardware = TRUE;
        return (void *)(uintptr_t)(PORTABLE_PLTT_BASE + (address - PORTABLE_PLTT_BASE));
    }
    if (address >= PORTABLE_VRAM_BASE && address < PORTABLE_VRAM_BASE + PORTABLE_VRAM_SIZE)
    {
        *rangeName = "vram";
        *isHardware = TRUE;
        return (void *)(uintptr_t)(PORTABLE_VRAM_BASE + (address - PORTABLE_VRAM_BASE));
    }
    if (address >= PORTABLE_OAM_BASE && address < PORTABLE_OAM_BASE + PORTABLE_OAM_SIZE)
    {
        *rangeName = "oam";
        *isHardware = TRUE;
        return (void *)(uintptr_t)(PORTABLE_OAM_BASE + (address - PORTABLE_OAM_BASE));
    }

    return (void *)ptr;
}

/*
 * PORTABLE: reject DMA that would extend past emulated hardware ranges (`0x18000` VRAM, etc.).
 * Field tile uploads: `LoadBgVram` → `RequestDma3Copy`; policy span `MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES`.
 */
static bool8 PortableDmaDestBufferRangeOk(const void *destAny, u32 size)
{
    const char *rangeName;
    bool8 isHardware;
    void *resolved;
    uintptr_t p;
    u32 off;

    resolved = PortableResolveDmaAddress(destAny, &rangeName, &isHardware);
    p = (uintptr_t)resolved;

    if (!isHardware)
        return TRUE;

    if (p >= PORTABLE_PLTT_BASE && p < PORTABLE_PLTT_BASE + PORTABLE_PLTT_SIZE)
    {
        off = (u32)(p - PORTABLE_PLTT_BASE);
        return (u32)(off + size) <= PORTABLE_PLTT_SIZE;
    }
    if (p >= PORTABLE_VRAM_BASE && p < PORTABLE_VRAM_BASE + PORTABLE_VRAM_SIZE)
    {
        off = (u32)(p - PORTABLE_VRAM_BASE);
        return (u32)(off + size) <= PORTABLE_VRAM_SIZE;
    }
    if (p >= PORTABLE_OAM_BASE && p < PORTABLE_OAM_BASE + PORTABLE_OAM_SIZE)
    {
        off = (u32)(p - PORTABLE_OAM_BASE);
        return (u32)(off + size) <= PORTABLE_OAM_SIZE;
    }

    return TRUE;
}

static void PortableTraceDmaRequest(const char *status, s16 index, const void *src, const void *dest, u16 size, u16 mode)
{
    char buffer[192];
    const char *destRange;
    bool8 destIsHardware;

    PortableResolveDmaAddress(dest, &destRange, &destIsHardware);
    snprintf(buffer, sizeof(buffer),
             "DMA req idx=%d status=%s src=%p dest=%p size=%u mode=%u hw=%u range=%s",
             index, status, src, dest, size, mode, destIsHardware, destRange);
    firered_runtime_trace_external(buffer);
}

static bool8 PortableTryImmediateDmaCopy(s16 index, const void *src, void *dest, u16 size, u16 mode)
{
    const char *srcRange;
    const char *destRange;
    bool8 srcIsHardware;
    bool8 destIsHardware;
    const void *resolvedSrc = PortableResolveDmaAddress(src, &srcRange, &srcIsHardware);
    void *resolvedDest = PortableResolveDmaAddress(dest, &destRange, &destIsHardware);

    if (!srcIsHardware && !destIsHardware)
        return FALSE;

    PortableTraceDmaRequest("pending-immediate", index, src, dest, size,
                            (mode == DMA3_32BIT) ? DMA_REQUEST_COPY32 : DMA_REQUEST_COPY16);
    if (mode == DMA3_32BIT)
    {
        CpuCopy32(resolvedSrc, resolvedDest, size);
    }
    else
    {
        CpuCopy16(resolvedSrc, resolvedDest, size);
    }
    PortableTraceDmaRequest("complete-immediate", index, src, dest, size,
                            (mode == DMA3_32BIT) ? DMA_REQUEST_COPY32 : DMA_REQUEST_COPY16);
    return TRUE;
}

static bool8 PortableTryImmediateDmaFill(s16 index, s32 value, void *dest, u16 size, u8 mode)
{
    const char *destRange;
    bool8 destIsHardware;
    void *resolvedDest = PortableResolveDmaAddress(dest, &destRange, &destIsHardware);

    if (!destIsHardware)
        return FALSE;

    PortableTraceDmaRequest("pending-immediate", index, NULL, dest, size,
                            (mode == DMA3_32BIT) ? DMA_REQUEST_FILL32 : DMA_REQUEST_FILL16);
    if (mode == DMA3_32BIT)
    {
        CpuFill32(value, resolvedDest, size);
    }
    else
    {
        CpuFill16(value, resolvedDest, size);
    }
    PortableTraceDmaRequest("complete-immediate", index, NULL, dest, size,
                            (mode == DMA3_32BIT) ? DMA_REQUEST_FILL32 : DMA_REQUEST_FILL16);
    return TRUE;
}
#endif

#define MAX_DMA_REQUESTS 128

static struct {
    /* 0x00 */ const u8 *src;
    /* 0x04 */ u8 *dest;
    /* 0x08 */ u16 size;
    /* 0x0A */ u16 mode;
    /* 0x0C */ u32 value;
} gDma3Requests[128];

static volatile bool8 gDma3ManagerLocked;
static u8 gDma3RequestCursor;

void ClearDma3Requests(void)
{
    int i;

    gDma3ManagerLocked = TRUE;
    gDma3RequestCursor = 0;

    for(i = 0; i < (u8)NELEMS(gDma3Requests); i++)
    {
        gDma3Requests[i].size = 0;
        gDma3Requests[i].src = 0;
        gDma3Requests[i].dest = 0;
    }

    gDma3ManagerLocked = FALSE;
}

void ProcessDma3Requests(void)
{
    u16 bytesTransferred;

    if (gDma3ManagerLocked)
        return;

    bytesTransferred = 0;

    // as long as there are DMA requests to process (unless size or vblank is an issue), do not exit
    while (gDma3Requests[gDma3RequestCursor].size != 0)
    {
#ifdef PORTABLE
        const void *resolvedSrc;
        void *resolvedDest;
        const char *srcRange;
        const char *destRange;
        bool8 srcIsHardware;
        bool8 destIsHardware;

        resolvedSrc = PortableResolveDmaAddress(gDma3Requests[gDma3RequestCursor].src, &srcRange, &srcIsHardware);
        resolvedDest = PortableResolveDmaAddress(gDma3Requests[gDma3RequestCursor].dest, &destRange, &destIsHardware);
        if (!PortableDmaDestBufferRangeOk(gDma3Requests[gDma3RequestCursor].dest, gDma3Requests[gDma3RequestCursor].size))
        {
            AGB_ASSERT(FALSE);
            gDma3Requests[gDma3RequestCursor].src = NULL;
            gDma3Requests[gDma3RequestCursor].dest = NULL;
            gDma3Requests[gDma3RequestCursor].size = 0;
            gDma3Requests[gDma3RequestCursor].mode = 0;
            gDma3Requests[gDma3RequestCursor].value = 0;
            gDma3RequestCursor++;
            if (gDma3RequestCursor >= MAX_DMA_REQUESTS)
                gDma3RequestCursor = 0;
            continue;
        }
        PortableTraceDmaRequest("pending", gDma3RequestCursor,
                                gDma3Requests[gDma3RequestCursor].src,
                                gDma3Requests[gDma3RequestCursor].dest,
                                gDma3Requests[gDma3RequestCursor].size,
                                gDma3Requests[gDma3RequestCursor].mode);
#endif
        bytesTransferred += gDma3Requests[gDma3RequestCursor].size;

        if (bytesTransferred > 40 * 1024)
            return; // don't transfer more than 40 KiB
#ifndef PORTABLE
        if (*(u8 *)REG_ADDR_VCOUNT > 224)
            return; // we're about to leave vblank, stop
#endif

        switch (gDma3Requests[gDma3RequestCursor].mode)
        {
        case DMA_REQUEST_COPY32: // regular 32-bit copy
#ifdef PORTABLE
            CpuCopy32(resolvedSrc,
                      resolvedDest,
                      gDma3Requests[gDma3RequestCursor].size);
#else
            Dma3CopyLarge32_(gDma3Requests[gDma3RequestCursor].src,
                             gDma3Requests[gDma3RequestCursor].dest,
                             gDma3Requests[gDma3RequestCursor].size);
#endif
            break;
        case DMA_REQUEST_FILL32: // repeat a single 32-bit value across RAM
#ifdef PORTABLE
            CpuFill32(gDma3Requests[gDma3RequestCursor].value,
                      resolvedDest,
                      gDma3Requests[gDma3RequestCursor].size);
#else
            Dma3FillLarge32_(gDma3Requests[gDma3RequestCursor].value,
                             gDma3Requests[gDma3RequestCursor].dest,
                             gDma3Requests[gDma3RequestCursor].size);
#endif
            break;
        case DMA_REQUEST_COPY16:    // regular 16-bit copy
#ifdef PORTABLE
            CpuCopy16(resolvedSrc,
                      resolvedDest,
                      gDma3Requests[gDma3RequestCursor].size);
#else
            Dma3CopyLarge16_(gDma3Requests[gDma3RequestCursor].src,
                             gDma3Requests[gDma3RequestCursor].dest,
                             gDma3Requests[gDma3RequestCursor].size);
#endif
            break;
        case DMA_REQUEST_FILL16: // repeat a single 16-bit value across RAM
#ifdef PORTABLE
            CpuFill16(gDma3Requests[gDma3RequestCursor].value,
                      resolvedDest,
                      gDma3Requests[gDma3RequestCursor].size);
#else
            Dma3FillLarge16_(gDma3Requests[gDma3RequestCursor].value,
                             gDma3Requests[gDma3RequestCursor].dest,
                             gDma3Requests[gDma3RequestCursor].size);
#endif
            break;
        }

        // Free the request
        gDma3Requests[gDma3RequestCursor].src = NULL;
        gDma3Requests[gDma3RequestCursor].dest = NULL;
        gDma3Requests[gDma3RequestCursor].size = 0;
        gDma3Requests[gDma3RequestCursor].mode = 0;
        gDma3Requests[gDma3RequestCursor].value = 0;
#ifdef PORTABLE
        PortableTraceDmaRequest("complete", gDma3RequestCursor,
                                gDma3Requests[gDma3RequestCursor].src,
                                gDma3Requests[gDma3RequestCursor].dest,
                                gDma3Requests[gDma3RequestCursor].size,
                                gDma3Requests[gDma3RequestCursor].mode);
#endif
        gDma3RequestCursor++;

        if (gDma3RequestCursor >= MAX_DMA_REQUESTS) // loop back to the first DMA request
            gDma3RequestCursor = 0;
    }
}

s16 RequestDma3Copy(const void *src, void *dest, u16 size, u8 mode)
{
    int cursor;
    int var = 0;

    gDma3ManagerLocked = 1;

#ifdef PORTABLE
    if (!PortableDmaDestBufferRangeOk(dest, size))
    {
        AGB_ASSERT(FALSE);
        gDma3ManagerLocked = FALSE;
        return -1;
    }
#endif

    cursor = gDma3RequestCursor;
    while(1)
    {
        if(!gDma3Requests[cursor].size) // an empty copy was found and the current cursor will be returned.
        {
#ifdef PORTABLE
            if (PortableTryImmediateDmaCopy((s16)cursor, src, dest, size, mode))
            {
                gDma3ManagerLocked = FALSE;
                return (s16)cursor;
            }
#endif
            gDma3Requests[cursor].src = src;
            gDma3Requests[cursor].dest = dest;
            gDma3Requests[cursor].size = size;

            if(mode == DMA3_32BIT)
                gDma3Requests[cursor].mode = DMA_REQUEST_COPY32;
            else
                gDma3Requests[cursor].mode = DMA_REQUEST_COPY16;

            gDma3ManagerLocked = FALSE;
            return (s16)cursor;
        }
        if(++cursor >= 0x80) // loop back to start.
        {
            cursor = 0;
        }
        if(++var >= 0x80) // max checks were made. all resulted in failure.
        {
            break;
        }
    }
    gDma3ManagerLocked = FALSE;
    return -1;
}

s16 RequestDma3Fill(s32 value, void *dest, u16 size, u8 mode)
{
    int cursor;
    int var = 0;

    cursor = gDma3RequestCursor;
    gDma3ManagerLocked = 1;

#ifdef PORTABLE
    if (!PortableDmaDestBufferRangeOk(dest, size))
    {
        AGB_ASSERT(FALSE);
        gDma3ManagerLocked = FALSE;
        return -1;
    }
#endif

    while(1)
    {
        if(!gDma3Requests[cursor].size)
        {
#ifdef PORTABLE
            if (PortableTryImmediateDmaFill((s16)cursor, value, dest, size, mode))
            {
                gDma3ManagerLocked = FALSE;
                return (s16)cursor;
            }
#endif
            gDma3Requests[cursor].dest = dest;
            gDma3Requests[cursor].size = size;
            gDma3Requests[cursor].mode = mode;
            gDma3Requests[cursor].value = value;

            if(mode == DMA3_32BIT)
                gDma3Requests[cursor].mode = DMA_REQUEST_FILL32;
            else
                gDma3Requests[cursor].mode = DMA_REQUEST_FILL16;

            gDma3ManagerLocked = FALSE;
            return (s16)cursor;
        }
        if(++cursor >= 0x80) // loop back to start.
        {
            cursor = 0;
        }
        if(++var >= 0x80) // max checks were made. all resulted in failure.
        {
            break;
        }
    }
    gDma3ManagerLocked = FALSE;
    return -1;
}

s16 WaitDma3Request(s16 index)
{
    int current = 0;

    if (index == -1)
    {
        for (; current < 0x80; current ++)
            if (gDma3Requests[current].size)
                return -1;

        return 0;
    }

    if (gDma3Requests[index].size)
    {
#ifdef PORTABLE
        PortableTraceDmaRequest("pending-check", index,
                                gDma3Requests[index].src,
                                gDma3Requests[index].dest,
                                gDma3Requests[index].size,
                                gDma3Requests[index].mode);
#endif
        return -1;

    }

    return 0;
}
