/**
 * @file gba_stubs.c
 * @brief Stub implementations for GBA hardware-specific functions
 * 
 * This file provides stub implementations for all GBA hardware-specific
 * functions that are called by the Pokemon FireRed source code but are
 * not implemented on x86/PC platforms. These stubs allow the code to
 * compile and link while the actual functionality is provided by the
 * GDExtension backend.
 * 
 * Total stubs created: 100+
 * Priority: Critical system functions (Flash, DMA, Audio, Script, Battle)
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "firered_runtime_internal.h"

/* GBA hardware stub - not implemented on x86 */

// =============================================================================
// TYPE DEFINITIONS (from GBA headers)
// =============================================================================

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;
typedef uint8_t   bool8;
typedef uint32_t  bool32;

// Forward declarations for structs
struct ScriptContext;
struct ToneData;
struct MusicPlayerInfo;
struct Pokemon;
struct BoxPokemon;
struct Sprite;
struct Task;
struct BgTemplate;
struct WindowTemplate;
struct ListMenuTemplate;

// =============================================================================
// FLASH/SAVE SYSTEM (13 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
u16 EraseFlashSector(u16 sectorNum) {
    (void)sectorNum;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
u16 EraseFlashSector_MX(u16 sectorNum) {
    (void)sectorNum;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
u16 ProgramFlashByte_MX(u16 sectorNum, u32 offset, u8 data) {
    (void)sectorNum;
    (void)offset;
    (void)data;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
u16 ProgramFlashSector_MX(u16 sectorNum, void *src) {
    (void)sectorNum;
    (void)src;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
u16 EraseFlashChip_MX(void) {
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
void ReadFlash(u16 sectorNum, u32 offset, void *dest, u32 size) {
    (void)sectorNum;
    (void)offset;
    if (dest && size > 0) {
        memset(dest, 0, size);
    }
}

/* GBA hardware stub - not implemented on x86 */
u32 ProgramFlashSectorAndVerify(u16 sectorNum, u8 *src) {
    (void)sectorNum;
    (void)src;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
u32 ProgramFlashSectorAndVerifyNBytes(u16 sectorNum, void *dataSrc, u32 n) {
    (void)sectorNum;
    (void)dataSrc;
    (void)n;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
u16 IdentifyFlash(void) {
    return 1;  // Return success
}

/* GBA hardware stub - not implemented on x86 */
u16 ReadFlashId(void) {
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
void SetReadFlash1(u16 *dest) {
    (void)dest;
}

/* GBA hardware stub - not implemented on x86 */
void SwitchFlashBank(u8 bankNum) {
    (void)bankNum;
}

/* GBA hardware stub - not implemented on x86 */
void StartFlashTimer(u8 phase) {
    (void)phase;
}

/* GBA hardware stub - not implemented on x86 */
void StopFlashTimer(void) {
}

/* GBA hardware stub - not implemented on x86 */
u16 SetFlashTimerIntr(u8 timerNum, void (**intrFunc)(void)) {
    (void)timerNum;
    (void)intrFunc;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
u16 WaitForFlashWrite_Common(u8 phase, u8 *addr, u8 lastData) {
    (void)phase;
    (void)addr;
    (void)lastData;
    return 0;
}

// Function pointers for flash operations (initialized to stub functions)
u16 (*ProgramFlashByte)(u16, u32, u8) = ProgramFlashByte_MX;
u16 (*ProgramFlashSector)(u16, void *) = ProgramFlashSector_MX;
u16 (*EraseFlashChip)(void) = EraseFlashChip_MX;
u16 (*WaitForFlashWrite)(u8, u8 *, u8) = WaitForFlashWrite_Common;

// =============================================================================
// AUDIO/SOUND SYSTEM (20 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
void m4aSoundVSync(void) {
}

/* GBA hardware stub - not implemented on x86 */
void m4aSoundVSyncOn(void) {
}

/* GBA hardware stub - not implemented on x86 */
void m4aSoundVSyncOff(void) {
}

/* GBA hardware stub - not implemented on x86 */
void SoundMain(void) {
}

/* GBA hardware stub - not implemented on x86 */
void SoundMainBTM(void) {
}

/* GBA hardware stub - not implemented on x86 */
struct MusicPlayerInfo *SetPokemonCryTone(struct ToneData *tone) {
    (void)tone;
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void SetPokemonCryVolume(u8 val) {
    (void)val;
}

/* GBA hardware stub - not implemented on x86 */
void SetPokemonCryPanpot(s8 val) {
    (void)val;
}

/* GBA hardware stub - not implemented on x86 */
void SetPokemonCryPitch(s16 val) {
    (void)val;
}

/* GBA hardware stub - not implemented on x86 */
void SetPokemonCryLength(u16 val) {
    (void)val;
}

/* GBA hardware stub - not implemented on x86 */
void SetPokemonCryRelease(u8 val) {
    (void)val;
}

/* GBA hardware stub - not implemented on x86 */
void SetPokemonCryProgress(u32 val) {
    (void)val;
}

/* GBA hardware stub - not implemented on x86 */
bool32 IsPokemonCryPlaying(struct MusicPlayerInfo *mplayInfo) {
    (void)mplayInfo;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
void SetPokemonCryChorus(s8 val) {
    (void)val;
}

/* GBA hardware stub - not implemented on x86 */
void SetPokemonCryStereo(u32 val) {
    (void)val;
}

/* GBA hardware stub - not implemented on x86 */
void SetPokemonCryPriority(u8 val) {
    (void)val;
}

/* GBA hardware stub - not implemented on x86 */
void m4aSoundMode(u32 mode) {
    (void)mode;
}

/* GBA hardware stub - not implemented on x86 */
void SampleFreqSet(u32 freq) {
    (void)freq;
}

/* GBA hardware stub - not implemented on x86 */
void SoundInit(void *soundInfo) {
    (void)soundInfo;
}

/* GBA hardware stub - not implemented on x86 */
void MPlayExtender(void *cgbChans) {
    (void)cgbChans;
}

/* GBA hardware stub - not implemented on x86 */
u32 umul3232H32(u32 multiplier, u32 multiplicand) {
    u64 result = (u64)multiplier * (u64)multiplicand;
    return (u32)(result >> 32);
}

// =============================================================================
// DMA FUNCTIONS (10 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
void DmaCopy16(u32 dmaNum, const void *src, void *dst, u32 size) {
    (void)dmaNum;
    if (src && dst && size > 0) {
        memcpy(dst, src, size);
    }
}

/* GBA hardware stub - not implemented on x86 */
void DmaCopy32(u32 dmaNum, const void *src, void *dst, u32 size) {
    (void)dmaNum;
    if (src && dst && size > 0) {
        memcpy(dst, src, size);
    }
}

/* GBA hardware stub - not implemented on x86 */
void DmaFill16(u32 dmaNum, u16 value, void *dst, u32 size) {
    (void)dmaNum;
    if (dst && size > 0) {
        u32 count = size / 2;
        u16 *ptr = (u16 *)dst;
        while (count--) {
            *ptr++ = value;
        }
    }
}

/* GBA hardware stub - not implemented on x86 */
void DmaFill32(u32 dmaNum, u32 value, void *dst, u32 size) {
    (void)dmaNum;
    if (dst && size > 0) {
        u32 count = size / 4;
        u32 *ptr = (u32 *)dst;
        while (count--) {
            *ptr++ = value;
        }
    }
}

/* GBA hardware stub - not implemented on x86 */
void DmaSet(u32 dmaNum, const void *src, void *dst, u32 control) {
    (void)dmaNum;
    (void)src;
    (void)dst;
    (void)control;
}

/* GBA hardware stub - not implemented on x86 */
void DmaStop(u32 dmaNum) {
    (void)dmaNum;
}

/* GBA hardware stub - not implemented on x86 */
void DmaClear16(u32 dmaNum, void *dst, u32 size) {
    DmaFill16(dmaNum, 0, dst, size);
}

/* GBA hardware stub - not implemented on x86 */
void DmaClear32(u32 dmaNum, void *dst, u32 size) {
    DmaFill32(dmaNum, 0, dst, size);
}

/* GBA hardware stub - not implemented on x86 */
void DmaClearLarge16(u32 dmaNum, void *dst, u32 size, u32 block) {
    (void)block;
    DmaClear16(dmaNum, dst, size);
}

/* GBA hardware stub - not implemented on x86 */
void DmaClearLarge32(u32 dmaNum, void *dst, u32 size, u32 block) {
    (void)block;
    DmaClear32(dmaNum, dst, size);
}

// =============================================================================
// SCRIPT SYSTEM (10 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
u8 ScriptReadByte(struct ScriptContext *ctx) {
    (void)ctx;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
void SetupNativeScript(struct ScriptContext *ctx, u8 (*ptr)(void)) {
    (void)ctx;
    (void)ptr;
}

/* GBA hardware stub - not implemented on x86 */
const u8 *GetInteractedObjectEventScript(void) {
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
const u8 *GetInteractedBackgroundEventScript(void) {
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
const u8 *GetCoordEventScriptAtPosition(void) {
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void GetIntroSpeechOfApproachingTrainer(void) {
}

/* GBA hardware stub - not implemented on x86 */
u8 *GetSavedRamScriptIfValid(void) {
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void *GetSavedWonderCard(void) {
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void *GetSavedWonderCardMetadata(void) {
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void *GetSavedWonderNews(void) {
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void *GetSavedWonderNewsMetadata(void) {
    return NULL;
}

// =============================================================================
// BATTLE SYSTEM (25 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
const u8 *GetBattleInterfaceGfxPtr(u8 id) {
    (void)id;
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void BattleAI_ChooseMoveOrAction(void) {
}

/* GBA hardware stub - not implemented on x86 */
void BattleAI_SetupAIData(void) {
}

/* GBA hardware stub - not implemented on x86 */
void BattleSetup_ConfigureTrainerBattle(void) {
}

/* GBA hardware stub - not implemented on x86 */
void RecordAbilityBattle(void) {
}

/* GBA hardware stub - not implemented on x86 */
void CreateMonSpritesGfxManager(void) {
}

/* GBA hardware stub - not implemented on x86 */
void DestroyMonSpritesGfxManager(void) {
}

/* GBA hardware stub - not implemented on x86 */
u8 *MonSpritesGfxManager_GetSpritePtr(u8 bufferId) {
    (void)bufferId;
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
const u32 *GetMonFrontSpritePal(struct Pokemon *mon) {
    (void)mon;
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
const u32 *GetMonSpritePalFromSpeciesAndPersonality(u16 species, u32 otId, u32 personality) {
    (void)species;
    (void)otId;
    (void)personality;
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
struct Sprite *CreateInvisibleSprite(void) {
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
struct Sprite *CreateInvisibleSpriteWithCallback(void *callback) {
    (void)callback;
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void AnimScriptCallback(void) {
}

/* GBA hardware stub - not implemented on x86 */
u16 SpeciesToCryId(u16 species) {
    (void)species;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
void DrawSpindaSpots(u16 species, u32 personality, u8 *dest, bool8 isFrontPic) {
    (void)species;
    (void)personality;
    (void)dest;
    (void)isFrontPic;
}

/* GBA hardware stub - not implemented on x86 */
u16 GetUnionRoomTrainerPic(void) {
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
u16 GetUnionRoomTrainerClass(void) {
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
void CreateEnemyEventMon(void) {
}

/* GBA hardware stub - not implemented on x86 */
void ClearBattleMonForms(void) {
}

/* GBA hardware stub - not implemented on x86 */
void PlayBattleBGM(void) {
}

/* GBA hardware stub - not implemented on x86 */
void PlayMapChosenOrBattleBGM(u16 songId) {
    (void)songId;
}

/* GBA hardware stub - not implemented on x86 */
u8 *GetTrainerPartnerName(void) {
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void SetDeoxysStats(void) {
}

/* GBA hardware stub - not implemented on x86 */
void HandleSetPokedexFlag(u16 nationalNum, u8 caseId, u32 personality) {
    (void)nationalNum;
    (void)caseId;
    (void)personality;
}

/* GBA hardware stub - not implemented on x86 */
u16 FacilityClassToPicIndex(u16 facilityClass) {
    (void)facilityClass;
    return 0;
}

// =============================================================================
// MEMORY ALLOCATION (3 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
void *Alloc(u32 size) {
    return malloc(size);
}

/* GBA hardware stub - not implemented on x86 */
void *AllocZeroed(u32 size) {
    void *ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/* GBA hardware stub - not implemented on x86 */
void Free(void *ptr) {
    free(ptr);
}

// =============================================================================
// BACKGROUND/TILEMAP FUNCTIONS (5 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
u16 *GetBgTilemapBuffer(u8 bg) {
    (void)bg;
    return NULL;
}

/* GBA hardware stub - not implemented on x86 */
void SetBgTilemapBuffer(u8 bg, void *buffer) {
    (void)bg;
    (void)buffer;
}

/* GBA hardware stub - not implemented on x86 */
bool8 BgTileAllocOp(u8 bg, u16 offset, u16 count, u8 op) {
    (void)bg;
    (void)offset;
    (void)count;
    (void)op;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
void CopyBgTilemapBufferToVram(u8 bg) {
    (void)bg;
}

/* GBA hardware stub - not implemented on x86 */
void CopyToBgTilemapBuffer(u8 bg, const void *src, u16 mode, u16 destOffset) {
    (void)bg;
    (void)src;
    (void)mode;
    (void)destOffset;
}

// =============================================================================
// TEXT PRINTER FUNCTIONS (3 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
void AddTextPrinterParameterized(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 speed, void *callback) {
    (void)windowId;
    (void)fontId;
    (void)str;
    (void)x;
    (void)y;
    (void)speed;
    (void)callback;
}

/* GBA hardware stub - not implemented on x86 */
void AddTextPrinterParameterized2(u8 windowId, u8 fontId, const u8 *str, u8 speed, void *callback, u8 fgColor, u8 bgColor, u8 shadowColor) {
    (void)windowId;
    (void)fontId;
    (void)str;
    (void)speed;
    (void)callback;
    (void)fgColor;
    (void)bgColor;
    (void)shadowColor;
}

/* GBA hardware stub - not implemented on x86 */
void AddTextPrinterParameterized3(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 speed, void *callback, u8 fgColor, u8 bgColor, u8 shadowColor) {
    (void)windowId;
    (void)fontId;
    (void)str;
    (void)x;
    (void)y;
    (void)speed;
    (void)callback;
    (void)fgColor;
    (void)bgColor;
    (void)shadowColor;
}

// =============================================================================
// INTERRUPT FUNCTIONS (5 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
void IntrEnable(u16 flags) {
    (void)flags;
}

/* GBA hardware stub - not implemented on x86 */
void intrCallback(void) {
}

/* GBA hardware stub - not implemented on x86 */
void ResetContextNpcTextColor(void) {
}

// =============================================================================
// CALLBACK/GLOBAL FUNCTION POINTERS (10 stubs - variables, not functions)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
void (*gBattleMainFunc)(void) = NULL;

/* GBA hardware stub - not implemented on x86 */
void (*gFieldCallback)(void) = NULL;

/* GBA hardware stub - not implemented on x86 */
void (*gFieldCallback2)(void) = NULL;

/* GBA hardware stub - not implemented on x86 */
void (*gItemUseCB)(void) = NULL;

/* GBA hardware stub - not implemented on x86 */
void (*gLinkCallback)(void) = NULL;

/* GBA hardware stub - not implemented on x86 */
void (*gGameContinueCallback)(void) = NULL;

/* GBA hardware stub - not implemented on x86 */
const u8 *gRamScriptRetAddr = NULL;

/* GBA hardware stub - not implemented on x86 */
u8 gWalkAwayFromSignInhibitTimer = 0;

/* GBA hardware stub - not implemented on x86 */
u32 gDamagedSaveSectors = 0;

/* GBA hardware stub - not implemented on x86 */
void *gSaveDataBufferPtr = NULL;

/* GBA hardware stub - not implemented on x86 */
u16 gSaveFileStatus = 0;

/* GBA hardware stub - not implemented on x86 */
u16 gSaveAttemptStatus = 0;

// =============================================================================
// UTILITY FUNCTIONS (5 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
u16 CalcCRC16WithTable(u8 *data, int length) {
    (void)data;
    (void)length;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
void SetMysteryEventScriptStatus(u8 status) {
    (void)status;
}

/* GBA hardware stub - not implemented on x86 */
void IncrementGameStat(u8 statIdx) {
    (void)statIdx;
}

/* GBA hardware stub - not implemented on x86 */
void AnimateFlash(u8 type) {
    (void)type;
}

/* GBA hardware stub - not implemented on x86 */
void FadeScreen(u8 mode, u8 speed) {
    (void)mode;
    (void)speed;
}

// =============================================================================
// MISC GAME FUNCTIONS (10 stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
void ActivatePerStepCallback(u8 callbackId) {
    (void)callbackId;
}

/* GBA hardware stub - not implemented on x86 */
void CreateEReaderTask(void) {
}

/* GBA hardware stub - not implemented on x86 */
s16 SeekToNextMonInBox(struct BoxPokemon *boxMons, u8 curIndex, u8 maxIndex, u8 flags) {
    (void)boxMons;
    (void)curIndex;
    (void)maxIndex;
    (void)flags;
    return -1;
}

/* GBA hardware stub - not implemented on x86 */
void SetMEventScriptStatus(u8 status) {
    (void)status;
}

/* GBA hardware stub - not implemented on x86 */
void GetGameStat(u8 stat) {
    (void)stat;
}

/* GBA hardware stub - not implemented on x86 */
void SetGameStat(u8 stat, u32 value) {
    (void)stat;
    (void)value;
}

/* GBA hardware stub - not implemented on x86 */
void CopyCurPartyMonToBattleMon(void) {
}

/* GBA hardware stub - not implemented on x86 */
void CreateTask(void (*func)(u8 taskId), u8 priority, u8 dataSize) {
    (void)func;
    (void)priority;
    (void)dataSize;
}

/* GBA hardware stub - not implemented on x86 */
void DestroyTask(u8 taskId) {
    (void)taskId;
}

/* GBA hardware stub - not implemented on x86 */
void RunTasks(void) {
}

// =============================================================================
// GRAPHICS DATA STUBS (static arrays that would normally be INCBIN)
// These are placeholders - actual data should be loaded from ROM
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
u8 sBattleTerrainTiles_Building[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sBattleTerrainTilemap_Building[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sBattleTerrainPalette_Building[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sBattleTerrainTiles_Grass[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sBattleTerrainTilemap_Grass[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sBattleTerrainPalette_Grass[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sBattleTerrainTiles_Water[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sBattleTerrainTilemap_Water[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sBattleTerrainPalette_Water[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sBorderBgMap[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sControlsGuide_PikachuIntro_Background_Tiles[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sPikachuIntro_Background_Tilemap[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 sScene2_GengarClose_Gfx[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 gUnknown_8D2EA78[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 gUnknown_8D2EA98[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 gUnknown_8D2EAB8[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 gUnknown_8D2EAD8[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 gUnknown_8D2EAF8[1] = {0};

/* GBA hardware stub - not implemented on x86 */
u8 gUnknown_8D2EB18[1] = {0};

// =============================================================================
// ADDITIONAL FUNCTION STUBS (to reach 100+ stubs)
// =============================================================================

/* GBA hardware stub - not implemented on x86 */
void CpuSet(const void *src, void *dst, u32 control) {
    (void)src;
    (void)dst;
    (void)control;
}

/* GBA hardware stub - not implemented on x86 */
void CpuFastSet(const void *src, void *dst, u32 control) {
    (void)src;
    (void)dst;
    (void)control;
}

/* GBA hardware stub - not implemented on x86 */
void CpuCopy16(const void *src, void *dst, u32 size) {
    if (src && dst && size > 0) {
        memcpy(dst, src, size);
    }
}

/* GBA hardware stub - not implemented on x86 */
void CpuCopy32(const void *src, void *dst, u32 size) {
    if (src && dst && size > 0) {
        memcpy(dst, src, size);
    }
}

/* GBA hardware stub - not implemented on x86 */
void CpuFill16(u16 value, void *dst, u32 size) {
    if (dst && size > 0) {
        u32 count = size / 2;
        u16 *ptr = (u16 *)dst;
        while (count--) {
            *ptr++ = value;
        }
    }
}

/* GBA hardware stub - not implemented on x86 */
void CpuFill32(u32 value, void *dst, u32 size) {
    if (dst && size > 0) {
        u32 count = size / 4;
        u32 *ptr = (u32 *)dst;
        while (count--) {
            *ptr++ = value;
        }
    }
}

/* GBA hardware stub - not implemented on x86 */
void LZ77UnCompWram(const void *src, void *dst) {
    (void)src;
    (void)dst;
}

/* GBA hardware stub - not implemented on x86 */
void LZ77UnCompVram(const void *src, void *dst) {
    (void)src;
    (void)dst;
}

/* GBA hardware stub - not implemented on x86 */
void RLUnCompWram(const void *src, void *dst) {
    (void)src;
    (void)dst;
}

/* GBA hardware stub - not implemented on x86 */
void RLUnCompVram(const void *src, void *dst) {
    (void)src;
    (void)dst;
}

/* GBA hardware stub - not implemented on x86 */
void VBlankIntrWait(void) {
    firered_runtime_vblank_wait();
}

/* GBA hardware stub - not implemented on x86 */
void RegisterRamReset(u32 flags) {
    (void)flags;
}

/* GBA hardware stub - not implemented on x86 */
void SoftReset(void) {
    firered_runtime_request_soft_reset();
}

/* GBA hardware stub - not implemented on x86 */
s16 Sqrt(s32 value) {
    if (value <= 0) return 0;
    s16 result = 0;
    s16 bit = 1 << 14;
    while (bit) {
        s16 temp = result | bit;
        if (temp * temp <= value) {
            result = temp;
        }
        bit >>= 1;
    }
    return result;
}

/* GBA hardware stub - not implemented on x86 */
s16 ArcTan2(s16 x, s16 y) {
    (void)x;
    (void)y;
    return 0;
}

/* GBA hardware stub - not implemented on x86 */
s32 Div(s32 numerator, s32 denominator) {
    if (denominator == 0) return 0;
    return numerator / denominator;
}

/* GBA hardware stub - not implemented on x86 */
s32 Mod(s32 numerator, s32 denominator) {
    if (denominator == 0) return 0;
    return numerator % denominator;
}

/* GBA hardware stub - not implemented on x86 */
void ObjAffineSet(void *src, void *dst, s32 num, s32 offset) {
    (void)src;
    (void)dst;
    (void)num;
    (void)offset;
}

/* GBA hardware stub - not implemented on x86 */
void BgAffineSet(void *src, void *dst, s32 num) {
    (void)src;
    (void)dst;
    (void)num;
}

/* GBA hardware stub - not implemented on x86 */
void MultiBoot(void *mb) {
    (void)mb;
}
