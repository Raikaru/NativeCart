#include "gba/gba.h"
#include "gba/flash_internal.h"

#include <string.h>

u8 *PortableFlash_GetSectorPointer(u16 sectorNum);
void PortableFlash_Flush(void);

#if defined(PORTABLE)
extern int PortableFlash_GetIoBatchDepth(void);
#define PORTABLE_FLASH_FLUSH_IF_NEEDED() \
    do { if (PortableFlash_GetIoBatchDepth() == 0) PortableFlash_Flush(); } while (0)
#else
#define PORTABLE_FLASH_FLUSH_IF_NEEDED() do { PortableFlash_Flush(); } while (0)
#endif

/* Erase sector buffer in RAM image only; caller decides when to persist to disk. */
static u16 EraseFlashSectorBuffer_MX(u16 sectorNum)
{
    u8 *dest;

    if (sectorNum >= gFlash->sector.count)
        return 0x80FF;

    if (gFlash->romSize == FLASH_ROM_SIZE_1M)
    {
        SwitchFlashBank(sectorNum / SECTORS_PER_BANK);
        sectorNum %= SECTORS_PER_BANK;
    }

    dest = PortableFlash_GetSectorPointer(sectorNum);
    if (dest == NULL)
        return 0x80FF;

    memset(dest, 0xFF, gFlash->sector.size);
    return 0;
}

const u16 mxMaxTime[] =
{
      10, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
      10, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
    2000, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
    2000, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
};

const struct FlashSetupInfo MX29L010 =
{
    ProgramFlashByte_MX,
    ProgramFlashSector_MX,
    EraseFlashChip_MX,
    EraseFlashSector_MX,
    WaitForFlashWrite_Common,
    mxMaxTime,
    {
        131072,
        {
            4096,
            12,
            32,
            0,
        },
        { 3, 1 },
        { { 0xC2, 0x09 } },
    }
};

const struct FlashSetupInfo DefaultFlash =
{
    ProgramFlashByte_MX,
    ProgramFlashSector_MX,
    EraseFlashChip_MX,
    EraseFlashSector_MX,
    WaitForFlashWrite_Common,
    mxMaxTime,
    {
        131072,
        {
            4096,
            12,
            32,
            0,
        },
        { 3, 1 },
        { { 0x00, 0x00 } },
    }
};

u16 EraseFlashChip_MX(void)
{
    u16 sectorNum;

    for (sectorNum = 0; sectorNum < gFlash->sector.count; sectorNum++)
        EraseFlashSector_MX(sectorNum);

    PortableFlash_Flush();
    return 0;
}

u16 EraseFlashSector_MX(u16 sectorNum)
{
    u8 *dest;

    if (sectorNum >= gFlash->sector.count)
        return 0x80FF;

    if (gFlash->romSize == FLASH_ROM_SIZE_1M)
    {
        SwitchFlashBank(sectorNum / SECTORS_PER_BANK);
        sectorNum %= SECTORS_PER_BANK;
    }

    dest = PortableFlash_GetSectorPointer(sectorNum);
    if (dest == NULL)
        return 0x80FF;

    memset(dest, 0xFF, gFlash->sector.size);
    PortableFlash_Flush();
    return 0;
}

u16 ProgramFlashByte_MX(u16 sectorNum, u32 offset, u8 data)
{
    u8 *dest;

    if (offset >= gFlash->sector.size)
        return 0x8000;

    if (gFlash->romSize == FLASH_ROM_SIZE_1M)
    {
        SwitchFlashBank(sectorNum / SECTORS_PER_BANK);
        sectorNum %= SECTORS_PER_BANK;
    }

    dest = PortableFlash_GetSectorPointer(sectorNum);
    if (dest == NULL)
        return 0x80FF;

    dest[offset] = data;
    PORTABLE_FLASH_FLUSH_IF_NEEDED();
    return 0;
}

u16 ProgramFlashSector_MX(u16 sectorNum, void *src)
{
    u8 *dest;
    u16 result;

    if (sectorNum >= gFlash->sector.count)
        return 0x80FF;

    result = EraseFlashSectorBuffer_MX(sectorNum);
    if (result != 0)
        return result;

    if (gFlash->romSize == FLASH_ROM_SIZE_1M)
    {
        SwitchFlashBank(sectorNum / SECTORS_PER_BANK);
        sectorNum %= SECTORS_PER_BANK;
    }

    dest = PortableFlash_GetSectorPointer(sectorNum);
    if (dest == NULL)
        return 0x80FF;

    memcpy(dest, src, gFlash->sector.size);
    gFlashNumRemainingBytes = 0;
    PORTABLE_FLASH_FLUSH_IF_NEEDED();
    return 0;
}
