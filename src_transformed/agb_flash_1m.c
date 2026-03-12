#include "gba/gba.h"
#include "gba/flash_internal.h"

extern const u16 mxMaxTime[];

u16 IdentifyFlash(void)
{
    ProgramFlashByte = ProgramFlashByte_MX;
    ProgramFlashSector = ProgramFlashSector_MX;
    EraseFlashChip = EraseFlashChip_MX;
    EraseFlashSector = EraseFlashSector_MX;
    WaitForFlashWrite = WaitForFlashWrite_Common;
    gFlashMaxTime = mxMaxTime;
    gFlash = &MX29L010.type;
    return 0;
}

u16 WaitForFlashWrite_Common(u8 phase, u8 *addr, u8 lastData)
{
    (void)phase;
    (void)addr;
    (void)lastData;
    return 0;
}
