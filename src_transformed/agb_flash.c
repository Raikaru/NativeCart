#include "gba/gba.h"
#include "gba/flash_internal.h"
#include "save.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORTABLE_FLASH_FILE_NAME "firered_save.sav"
#define PORTABLE_FLASH_PATH_SIZE 1024

COMMON_DATA u8 gFlashTimeoutFlag = 0;
COMMON_DATA u8 (*PollFlashStatus)(u8 *) = NULL;
COMMON_DATA u16 (*WaitForFlashWrite)(u8 phase, u8 *addr, u8 lastData) = NULL;
COMMON_DATA u16 (*ProgramFlashSector)(u16 sectorNum, void *src) = NULL;
COMMON_DATA const struct FlashType *gFlash = NULL;
COMMON_DATA u16 (*ProgramFlashByte)(u16 sectorNum, u32 offset, u8 data) = NULL;
COMMON_DATA u16 gFlashNumRemainingBytes = 0;
COMMON_DATA u16 (*EraseFlashChip)(void) = NULL;
COMMON_DATA u16 (*EraseFlashSector)(u16 sectorNum) = NULL;
COMMON_DATA const u16 *gFlashMaxTime = NULL;

static bool8 sPortableFlashLoaded = FALSE;
static u8 sPortableFlashBank = 0;
static u8 sPortableFlashData[FLASH_ROM_SIZE_1M];

static void PortableFlash_Reset(void)
{
    memset(sPortableFlashData, 0xFF, sizeof(sPortableFlashData));
}

static void PortableFlash_GetPath(char *path, size_t pathSize)
{
    const char *base;

#ifdef _WIN32
    base = getenv("APPDATA");
    if (base != NULL && base[0] != '\0')
    {
        snprintf(path, pathSize, "%s\\Godot\\app_userdata\\GodotFireRed\\%s", base, PORTABLE_FLASH_FILE_NAME);
        return;
    }
#elif defined(__APPLE__)
    base = getenv("HOME");
    if (base != NULL && base[0] != '\0')
    {
        snprintf(path, pathSize, "%s/Library/Application Support/Godot/app_userdata/GodotFireRed/%s", base, PORTABLE_FLASH_FILE_NAME);
        return;
    }
#else
    base = getenv("HOME");
    if (base != NULL && base[0] != '\0')
    {
        snprintf(path, pathSize, "%s/.local/share/godot/app_userdata/GodotFireRed/%s", base, PORTABLE_FLASH_FILE_NAME);
        return;
    }
#endif

    snprintf(path, pathSize, "%s", PORTABLE_FLASH_FILE_NAME);
}

static void PortableFlash_Load(void)
{
    char path[PORTABLE_FLASH_PATH_SIZE];
    FILE *file;
    size_t bytesRead;

    if (sPortableFlashLoaded)
        return;

    PortableFlash_Reset();
    PortableFlash_GetPath(path, sizeof(path));
    file = fopen(path, "rb");
    if (file != NULL)
    {
        bytesRead = fread(sPortableFlashData, 1, sizeof(sPortableFlashData), file);
        if (bytesRead < sizeof(sPortableFlashData))
            memset(sPortableFlashData + bytesRead, 0xFF, sizeof(sPortableFlashData) - bytesRead);
        fclose(file);
    }

    sPortableFlashLoaded = TRUE;
}

void PortableFlash_Flush(void)
{
    char path[PORTABLE_FLASH_PATH_SIZE];
    FILE *file;

    PortableFlash_Load();
    PortableFlash_GetPath(path, sizeof(path));
    file = fopen(path, "wb");
    if (file != NULL)
    {
        fwrite(sPortableFlashData, sizeof(sPortableFlashData), 1, file);
        fclose(file);
    }
}

u8 *PortableFlash_GetSectorPointer(u16 sectorNum)
{
    u32 physicalSector = sectorNum;

    PortableFlash_Load();

    if (gFlash != NULL && gFlash->romSize == FLASH_ROM_SIZE_1M)
        physicalSector += sPortableFlashBank * SECTORS_PER_BANK;

    if (physicalSector >= (FLASH_ROM_SIZE_1M >> 12))
        return NULL;

    return sPortableFlashData + (physicalSector << 12);
}

void SwitchFlashBank(u8 bankNum)
{
    sPortableFlashBank = bankNum;
}

u16 ReadFlashId(void)
{
    return 0x09C2;
}

u16 SetFlashTimerIntr(u8 timerNum, void (**intrFunc)(void))
{
    (void)timerNum;
    (void)intrFunc;
    return 0;
}

void StartFlashTimer(u8 phase)
{
    (void)phase;
}

void StopFlashTimer(void)
{
}

void SetReadFlash1(u16 *dest)
{
    (void)dest;
}

void ReadFlash(u16 sectorNum, u32 offset, void *dest, u32 size)
{
    u8 *src;

    if (dest == NULL || size == 0)
        return;

    if (gFlash != NULL && gFlash->romSize == FLASH_ROM_SIZE_1M)
    {
        SwitchFlashBank(sectorNum / SECTORS_PER_BANK);
        sectorNum %= SECTORS_PER_BANK;
    }

    src = PortableFlash_GetSectorPointer(sectorNum);
    if (src == NULL || offset >= SECTOR_SIZE)
    {
        memset(dest, 0xFF, size);
        return;
    }

    if (offset + size > SECTOR_SIZE)
        size = SECTOR_SIZE - offset;

    memcpy(dest, src + offset, size);
}

static u32 VerifyFlashSectorInternal(u16 sectorNum, u8 *src, u32 size)
{
    u8 *dest;

    if (gFlash != NULL && gFlash->romSize == FLASH_ROM_SIZE_1M)
    {
        SwitchFlashBank(sectorNum / SECTORS_PER_BANK);
        sectorNum %= SECTORS_PER_BANK;
    }

    dest = PortableFlash_GetSectorPointer(sectorNum);
    if (dest == NULL)
        return 1;

    if (memcmp(dest, src, size) != 0)
        return 1;

    return 0;
}

u32 ProgramFlashSectorAndVerify(u16 sectorNum, u8 *src)
{
    u32 result;

    result = ProgramFlashSector(sectorNum, src);
    if (result != 0)
        return result;

    return VerifyFlashSectorInternal(sectorNum, src, gFlash->sector.size);
}

u32 ProgramFlashSectorAndVerifyNBytes(u16 sectorNum, void *dataSrc, u32 n)
{
    u32 result;

    result = ProgramFlashSector(sectorNum, dataSrc);
    if (result != 0)
        return result;

    return VerifyFlashSectorInternal(sectorNum, dataSrc, n);
}
