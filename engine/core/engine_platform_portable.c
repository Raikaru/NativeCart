#include "../../include/global.h"
#include "../../include/gba/syscall.h"
#include "../../include/gba_malloc.h"
#include "../../include/trig.h"

#include "engine_internal.h"
#include "engine_runtime_internal.h"

#include <math.h>
#include <string.h>

extern void firered_portable_m4aSoundVSync(void);

#define ENGINE_PI 3.14159265358979323846

u8 gHeap[HEAP_SIZE];

static void engine_memzero_region(uintptr_t address, size_t size)
{
    memset((void *)address, 0, size);
}

static void engine_lz77_uncomp(const u8 *src, u8 *dest)
{
    u32 remaining;
    u32 written = 0;
    u8 header;

    if (src == NULL || dest == NULL)
        return;

    header = *src++;
    if (header != 0x10)
        return;

    remaining = src[0] | (src[1] << 8) | (src[2] << 16);
    src += 3;

    while (written < remaining)
    {
        u8 flags = *src++;
        s32 i;

        for (i = 0; i < 8 && written < remaining; i++, flags <<= 1)
        {
            if (flags & 0x80)
            {
                u8 hi = *src++;
                u8 lo = *src++;
                u32 count = (hi >> 4) + 3;
                u32 disp = (((hi & 0xF) << 8) | lo) + 1;

                while (count-- != 0 && written < remaining)
                {
                    dest[written] = (disp <= written) ? dest[written - disp] : 0;
                    written++;
                }
            }
            else
            {
                dest[written++] = *src++;
            }
        }
    }
}

void RegisterRamReset(u32 resetFlags)
{
    if (resetFlags & RESET_EWRAM)
        engine_memzero_region(ENGINE_EWRAM_ADDR, ENGINE_EWRAM_SIZE);
    if (resetFlags & RESET_IWRAM)
        engine_memzero_region(ENGINE_IWRAM_ADDR, ENGINE_IWRAM_SIZE);
    if (resetFlags & RESET_PALETTE)
        engine_memzero_region(ENGINE_PALETTE_ADDR, ENGINE_PALETTE_SIZE);
    if (resetFlags & RESET_VRAM)
        engine_memzero_region(ENGINE_VRAM_ADDR, ENGINE_VRAM_SIZE);
    if (resetFlags & RESET_OAM)
        engine_memzero_region(ENGINE_OAM_ADDR, ENGINE_OAM_SIZE);
    if (resetFlags & (RESET_SIO_REGS | RESET_SOUND_REGS | RESET_REGS))
        engine_memzero_region(ENGINE_IOREG_ADDR, ENGINE_IOREG_SIZE);
}

void SoftReset(u32 resetFlags)
{
    RegisterRamReset(resetFlags);
    engine_backend_request_soft_reset();
}

void CpuSet(const void *src, void *dest, u32 control)
{
    u32 count = control & 0x1FFFFF;

    if (control & CPU_SET_32BIT)
    {
        const u32 *src32 = src;
        u32 *dest32 = dest;
        u32 value = *src32;

        while (count-- != 0)
        {
            *dest32++ = value;
            if (!(control & CPU_SET_SRC_FIXED))
                value = *++src32;
        }
    }
    else
    {
        const u16 *src16 = src;
        u16 *dest16 = dest;
        u16 value = *src16;

        while (count-- != 0)
        {
            *dest16++ = value;
            if (!(control & CPU_SET_SRC_FIXED))
                value = *++src16;
        }
    }
}

void CpuFastSet(const void *src, void *dest, u32 control)
{
    u32 count = control & 0x1FFFFF;
    const u32 *src32 = src;
    u32 *dest32 = dest;
    u32 value = *src32;

    while (count-- != 0)
    {
        *dest32++ = value;
        if (!(control & CPU_FAST_SET_SRC_FIXED))
            value = *++src32;
    }
}

void BgAffineSet(struct BgAffineSrcData *src, struct BgAffineDstData *dest, s32 count)
{
    while (count-- > 0)
    {
        s16 sin = Sin((u8)src->alpha, 256);
        s16 cos = Cos((u8)src->alpha, 256);

        dest->pa = (src->sx * cos) >> 8;
        dest->pb = -((src->sx * sin) >> 8);
        dest->pc = (src->sy * sin) >> 8;
        dest->pd = (src->sy * cos) >> 8;
        dest->dx = src->texX - (dest->pa * src->scrX + dest->pb * src->scrY);
        dest->dy = src->texY - (dest->pc * src->scrX + dest->pd * src->scrY);

        src++;
        dest++;
    }
}

void ObjAffineSet(struct ObjAffineSrcData *src, void *dest, s32 count, s32 offset)
{
    u8 *dest8 = dest;

    while (count-- > 0)
    {
        s16 sin = Sin((u8)(src->rotation >> 8), 256);
        s16 cos = Cos((u8)(src->rotation >> 8), 256);
        s16 pa = (src->xScale * cos) >> 8;
        s16 pb = -((src->xScale * sin) >> 8);
        s16 pc = (src->yScale * sin) >> 8;
        s16 pd = (src->yScale * cos) >> 8;

        *(s16 *)(dest8 + offset * 0) = pa;
        *(s16 *)(dest8 + offset * 1) = pb;
        *(s16 *)(dest8 + offset * 2) = pc;
        *(s16 *)(dest8 + offset * 3) = pd;

        src++;
        dest8 += offset * 4;
    }
}

void LZ77UnCompWram(const void *src, void *dest)
{
    engine_lz77_uncomp(src, dest);
}

void LZ77UnCompVram(const void *src, void *dest)
{
#ifdef PORTABLE
    engine_backend_trace_external("LZ77UnCompVram: enter");
#endif
    engine_lz77_uncomp(src, dest);
#ifdef PORTABLE
    engine_backend_trace_external("LZ77UnCompVram: exit");
#endif
}

u16 Sqrt(u32 num)
{
    u32 bit = 1u << 30;
    u32 result = 0;

    while (bit > num)
        bit >>= 2;

    while (bit != 0)
    {
        if (num >= result + bit)
        {
            num -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }

        bit >>= 2;
    }

    return (u16)result;
}

u16 ArcTan2(s16 x, s16 y)
{
    double angle = atan2((double)y, (double)x);
    long scaled = lround(angle * (32768.0 / ENGINE_PI));

    if (scaled < 0)
        scaled += 65536;

    return (u16)scaled;
}

void VBlankIntrWait(void)
{
    engine_backend_vblank_wait();
}

s32 Div(s32 num, s32 denom)
{
    if (denom == 0)
        return 0;

    return num / denom;
}

int MultiBoot(struct MultiBootParam *mp)
{
    (void)mp;
    return 0;
}

void m4aSoundVSync(void)
{
    firered_portable_m4aSoundVSync();
}
