#include "gba/gba.h"
#include "gba/flash_internal.h"
#include "save.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

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

static void PortableFlash_Load(void);

#ifdef PORTABLE
static int s_portable_flash_io_batch_depth;

void PortableFlash_BeginIoBatch(void)
{
    PortableFlash_Load();
    s_portable_flash_io_batch_depth++;
}

void PortableFlash_EndIoBatch(void)
{
    if (s_portable_flash_io_batch_depth > 0)
        s_portable_flash_io_batch_depth--;
    if (s_portable_flash_io_batch_depth == 0)
        PortableFlash_Flush();
}

int PortableFlash_GetIoBatchDepth(void)
{
    return s_portable_flash_io_batch_depth;
}
#endif

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

static void PortableFlash_EnsureParentDirExists(const char *path)
{
    char temp[PORTABLE_FLASH_PATH_SIZE];
    size_t i;

    if (path == NULL || path[0] == '\0')
        return;

    memset(temp, 0, sizeof(temp));
    strncpy(temp, path, sizeof(temp) - 1);

    for (i = 0; temp[i] != '\0'; i++)
    {
        if (temp[i] != '/' && temp[i] != '\\')
            continue;

        if (i == 0)
            continue;
#ifdef _WIN32
        if (i == 2 && temp[1] == ':')
            continue;
#endif

        temp[i] = '\0';
#ifdef _WIN32
        _mkdir(temp);
#else
        mkdir(temp, 0755);
#endif
        temp[i] = path[i];
    }
}

void PortableFlash_Flush(void)
{
    char path[PORTABLE_FLASH_PATH_SIZE];
    FILE *file;

    PortableFlash_Load();
    PortableFlash_GetPath(path, sizeof(path));
    PortableFlash_EnsureParentDirExists(path);
    file = fopen(path, "wb");
    if (file != NULL)
    {
        fwrite(sPortableFlashData, sizeof(sPortableFlashData), 1, file);
        fclose(file);
    }
}

#ifdef PORTABLE
void PortableFlash_Export(const char *path)
{
    FILE *file;

    if (path == NULL || path[0] == '\0')
        return;

    PortableFlash_Load();
    PortableFlash_EnsureParentDirExists(path);

    file = fopen(path, "wb");
    if (file == NULL)
        return;

    // FireRed (and this portable build) uses a 1Mbit flash chip:
    // export the raw 128KiB image so mGBA can treat it as battery-backed flash.
    fwrite(sPortableFlashData, sizeof(sPortableFlashData), 1, file);
    fclose(file);
}

#ifdef FIRERED
/*
 * PKHeX's SaveUtil.IsG3 returns the *first* main slot that has all 14 sectors, not the newest.
 * GetVersionG3SAV then reads u32 LE at SaveBlock2+0xAC from that slot. If slot 0 was saved
 * before we wrote the FRLG fingerprint but slot 1 is newer, PKHeX still inspects slot 0 and
 * mis-detects Ruby/Sapphire. Patch every SaveBlock2 sector (footer id == 0) in both slots.
 */
static u16 PortableFlash_CalculateSectorPayloadChecksum(const u8 *data, u16 size)
{
    u32 checksum = 0;
    u16 i;
    u16 n = (u16)(size / 4);

    for (i = 0; i < n; i++)
    {
        u32 word;
        memcpy(&word, data + i * 4, sizeof(word));
        checksum += word;
    }

    return (u16)((checksum >> 16) + checksum);
}

void PortableFlash_PatchPkhexFrLgFingerprintAllSlots(void)
{
    const u32 sizeMain = NUM_SECTORS_PER_SLOT * SECTOR_SIZE;
    u32 slot;
    bool8 modified = FALSE;

    PortableFlash_Load();

    for (slot = 0; slot < NUM_SAVE_SLOTS; slot++)
    {
        u32 base = slot * sizeMain;
        u16 i;

        for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
        {
            u8 *sector = sPortableFlashData + base + (u32)i * SECTOR_SIZE;
            u16 secId = sector[offsetof(struct SaveSector, id)]
                      | ((u16)sector[offsetof(struct SaveSector, id) + 1] << 8);
            u32 sig = sector[offsetof(struct SaveSector, signature)]
                    | ((u32)sector[offsetof(struct SaveSector, signature) + 1] << 8)
                    | ((u32)sector[offsetof(struct SaveSector, signature) + 2] << 16)
                    | ((u32)sector[offsetof(struct SaveSector, signature) + 3] << 24);
            u16 payloadSize = (u16)sizeof(struct SaveBlock2);

            if (payloadSize > SECTOR_DATA_SIZE)
                payloadSize = SECTOR_DATA_SIZE;

            if (secId != SECTOR_ID_SAVEBLOCK2)
                continue;
            if (sig != SECTOR_SIGNATURE)
                continue;

            if (sector[0xAC] != 1 || sector[0xAD] != 0 || sector[0xAE] != 0 || sector[0xAF] != 0)
                modified = TRUE;

            sector[0xAC] = 1;
            sector[0xAD] = 0;
            sector[0xAE] = 0;
            sector[0xAF] = 0;

            {
                u16 chk = PortableFlash_CalculateSectorPayloadChecksum(sector, payloadSize);
                u16 *chkField = (u16 *)(sector + offsetof(struct SaveSector, checksum));

                if (*chkField != chk)
                    modified = TRUE;
                *chkField = chk;
            }
        }
    }

    if (modified)
        PortableFlash_Flush();
}
#endif /* FIRERED */
#endif /* PORTABLE */

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
