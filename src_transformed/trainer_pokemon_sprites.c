#include "global.h"
#include "gflib.h"
#include "decompress.h"
#include "data.h"

#ifdef PORTABLE
#include <stdio.h>

#include "trainer_graphics_portable_paths.h"
#endif

struct PicData
{
    u8 *frames;
    struct SpriteFrameImage *images;
    u16 paletteTag;
    u8 spriteId;
    u8 active;
};

#define PICS_COUNT 8

static EWRAM_DATA struct SpriteTemplate sCreatingSpriteTemplate = {};
static EWRAM_DATA struct PicData sSpritePics[PICS_COUNT] = {};

static const struct PicData sDummyPicData = {};

#ifdef PORTABLE
static const char *const sPortableTrainerAssetPrefixes[] = {
    "",
    "../",
    "../../",
    "../../../",
    "../../../../",
};

static EWRAM_DATA const u32 *sPortableTrainerFrontPicData[ARRAY_COUNT(sTrainerFrontPicPaths_Portable)] = {};
static EWRAM_DATA const u32 *sPortableTrainerFrontPaletteData[ARRAY_COUNT(sTrainerFrontPalettePaths_Portable)] = {};

static void *LoadPortableTrainerAssetFile(const char *relativePath)
{
    FILE *file;
    void *data;
    char fullPath[512];
    size_t bytesRead;
    long size;
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sPortableTrainerAssetPrefixes); i++)
    {
        snprintf(fullPath, sizeof(fullPath), "%s%s", sPortableTrainerAssetPrefixes[i], relativePath);
        file = fopen(fullPath, "rb");
        if (file != NULL)
            break;
    }

    if (i == ARRAY_COUNT(sPortableTrainerAssetPrefixes))
        return NULL;
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }

    size = ftell(file);
    if (size <= 0)
    {
        fclose(file);
        return NULL;
    }

    rewind(file);
    data = Alloc((u32)size);
    if (data == NULL)
    {
        fclose(file);
        return NULL;
    }

    bytesRead = fread(data, 1, (size_t)size, file);
    fclose(file);
    if (bytesRead != (size_t)size)
    {
        Free(data);
        return NULL;
    }

    return data;
}

static const u32 *GetPortableTrainerFrontPicData(u16 frontPicId)
{
    if (frontPicId >= ARRAY_COUNT(sTrainerFrontPicPaths_Portable))
        return NULL;
    if (sPortableTrainerFrontPicData[frontPicId] == NULL)
        sPortableTrainerFrontPicData[frontPicId] = LoadPortableTrainerAssetFile(sTrainerFrontPicPaths_Portable[frontPicId]);
    return sPortableTrainerFrontPicData[frontPicId];
}

static const u32 *GetPortableTrainerFrontPaletteData(u16 frontPicId)
{
    if (frontPicId >= ARRAY_COUNT(sTrainerFrontPalettePaths_Portable))
        return NULL;
    if (sPortableTrainerFrontPaletteData[frontPicId] == NULL)
        sPortableTrainerFrontPaletteData[frontPicId] = LoadPortableTrainerAssetFile(sTrainerFrontPalettePaths_Portable[frontPicId]);
    return sPortableTrainerFrontPaletteData[frontPicId];
}
#endif

static const struct OamData sOamData_Normal =
{
    .shape = SPRITE_SHAPE(64x64),
    .size = SPRITE_SIZE(64x64)
};

void DummyPicSpriteCallback(struct Sprite *sprite)
{

}

bool16 ResetAllPicSprites(void)
{
    int i;

    for (i = 0; i < PICS_COUNT; i ++)
        sSpritePics[i] = sDummyPicData;

    return FALSE;
}

static bool16 DecompressPic(u16 species, u32 personality, bool8 isFrontPic, u8 *dest, bool8 isTrainer, bool8 ignoreDeoxys)
{
    if (!isTrainer)
    {
        if (isFrontPic)
        {
            if (!ignoreDeoxys)
                LoadSpecialPokePic(&gMonFrontPicTable[species], dest, species, personality, isFrontPic);
            else
                LoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[species], dest, species, personality, isFrontPic);
        }
        else
        {
            if (!ignoreDeoxys)
                LoadSpecialPokePic(&gMonBackPicTable[species], dest, species, personality, isFrontPic);
            else
                LoadSpecialPokePic_DontHandleDeoxys(&gMonBackPicTable[species], dest, species, personality, isFrontPic);
        }
    }
    else
    {
        if (isFrontPic)
#ifdef PORTABLE
        {
            const u32 *picData = GetPortableTrainerFrontPicData(species);

            if (picData != NULL)
                LZ77UnCompWram(picData, dest);
            else
                DecompressPicFromTable(&gTrainerFrontPicTable[species], dest, species);
        }
#else
            DecompressPicFromTable(&gTrainerFrontPicTable[species], dest, species);
#endif
        else
            DecompressPicFromTable(&gTrainerBackPicTable[species], dest, species);
    }
    return FALSE;
}

static bool16 DecompressPic_HandleDeoxys(u16 species, u32 personality, bool8 isFrontPic, u8 *dest, bool8 isTrainer)
{
    return DecompressPic(species, personality, isFrontPic, dest, isTrainer, FALSE);
}

void LoadPicPaletteByTagOrSlot(u16 species, u32 otId, u32 personality, u8 paletteSlot, u16 paletteTag, bool8 isTrainer)
{
    if (!isTrainer)
    {
        if (paletteTag == TAG_NONE)
        {
            sCreatingSpriteTemplate.paletteTag = TAG_NONE;
            LoadCompressedPalette(GetMonSpritePalFromSpeciesAndPersonality(species, otId, personality), OBJ_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
        }
        else
        {
            sCreatingSpriteTemplate.paletteTag = paletteTag;
            LoadCompressedSpritePalette(GetMonSpritePalStructFromOtIdPersonality(species, otId, personality));
        }
    }
    else
    {
        if (paletteTag == TAG_NONE)
        {
            sCreatingSpriteTemplate.paletteTag = TAG_NONE;
#ifdef PORTABLE
            {
                const u32 *paletteData = GetPortableTrainerFrontPaletteData(species);

                if (paletteData != NULL)
                    LoadCompressedPalette(paletteData, OBJ_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
                else
                    LoadCompressedPalette(gTrainerFrontPicPaletteTable[species].data, OBJ_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
            }
#else
            LoadCompressedPalette(gTrainerFrontPicPaletteTable[species].data, OBJ_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
#endif
        }
        else
        {
            sCreatingSpriteTemplate.paletteTag = paletteTag;
            LoadCompressedSpritePalette(&gTrainerFrontPicPaletteTable[species]);
        }
    }
}

void LoadPicPaletteBySlot(u16 species, u32 otId, u32 personality, u8 paletteSlot, bool8 isTrainer)
{
    if (!isTrainer)
        LoadCompressedPalette(GetMonSpritePalFromSpeciesAndPersonality(species, otId, personality), BG_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
    else
    {
#ifdef PORTABLE
        const u32 *paletteData = GetPortableTrainerFrontPaletteData(species);

        if (paletteData != NULL)
            LoadCompressedPalette(paletteData, BG_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
        else
            LoadCompressedPalette(gTrainerFrontPicPaletteTable[species].data, BG_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
#else
        LoadCompressedPalette(gTrainerFrontPicPaletteTable[species].data, BG_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
#endif
    }
}

void AssignSpriteAnimsTable(bool8 isTrainer)
{
    if (!isTrainer)
        sCreatingSpriteTemplate.anims = gAnims_MonPic;
    else
        sCreatingSpriteTemplate.anims = gTrainerFrontAnimsPtrTable[0];
}

u16 CreatePicSprite(u16 species, u32 otId, u32 personality, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag, bool8 isTrainer, bool8 ignoreDeoxys)
{
    u8 i;
    u8 *framePics;
    struct SpriteFrameImage *images;
    int j;
    u8 spriteId;

    for (i = 0; i < PICS_COUNT; i ++)
    {
        if (!sSpritePics[i].active)
            break;
    }
    if (i == PICS_COUNT)
        return 0xFFFF;

    framePics = Alloc(4 * 0x800);
    if (!framePics)
        return 0xFFFF;

    images = Alloc(4 * sizeof(struct SpriteFrameImage));
    if (!images)
    {
        Free(framePics);
        return 0xFFFF;
    }
    if (DecompressPic(species, personality, isFrontPic, framePics, isTrainer, ignoreDeoxys))
    {
        // debug trap?
        return 0xFFFF;
    }
    for (j = 0; j < 4; j ++)
    {
        images[j].data = framePics + 0x800 * j;
        images[j].size = 0x800;
    }
    sCreatingSpriteTemplate.tileTag = TAG_NONE;
    sCreatingSpriteTemplate.oam = &sOamData_Normal;
    AssignSpriteAnimsTable(isTrainer);
    sCreatingSpriteTemplate.images = images;
    sCreatingSpriteTemplate.affineAnims = gDummySpriteAffineAnimTable;
    sCreatingSpriteTemplate.callback = DummyPicSpriteCallback;
    LoadPicPaletteByTagOrSlot(species, otId, personality, paletteSlot, paletteTag, isTrainer);
    spriteId = CreateSprite(&sCreatingSpriteTemplate, x, y, 0);
    if (spriteId == MAX_SPRITES)
    {
        if (paletteTag != TAG_NONE)
            FreeSpritePaletteByTag(paletteTag);
        Free(framePics);
        Free(images);
        return 0xFFFF;
    }

    if (paletteTag == TAG_NONE)
        gSprites[spriteId].oam.paletteNum = paletteSlot;
    sSpritePics[i].frames = framePics;
    sSpritePics[i].images = images;
    sSpritePics[i].paletteTag = paletteTag;
    sSpritePics[i].spriteId = spriteId;
    sSpritePics[i].active = TRUE;
    return spriteId;
}

u16 CreatePicSprite_HandleDeoxys(u16 species, u32 otId, u32 personality, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag, bool8 isTrainer)
{
    return CreatePicSprite(species, otId, personality, isFrontPic, x, y, paletteSlot, paletteTag, isTrainer, FALSE);
}

u16 FreeAndDestroyPicSpriteInternal(u16 spriteId)
{
    u8 i;
    u8 *framePics;
    struct SpriteFrameImage *images;

    for (i = 0; i < PICS_COUNT; i ++)
    {
        if (sSpritePics[i].spriteId == spriteId)
            break;
    }
    if (i == PICS_COUNT)
        return 0xFFFF;

    framePics = sSpritePics[i].frames;
    images = sSpritePics[i].images;
    if (sSpritePics[i].paletteTag != TAG_NONE)
        FreeSpritePaletteByTag(GetSpritePaletteTagByPaletteNum(gSprites[spriteId].oam.paletteNum));
    DestroySprite(&gSprites[spriteId]);
    Free(framePics);
    Free(images);
    sSpritePics[i] = sDummyPicData;
    return 0;
}

static u16 LoadPicSpriteInWindow(u16 species, u32 otId, u32 personality, bool8 isFrontPic, u8 paletteSlot, u8 windowId, bool8 isTrainer)
{
    if (DecompressPic_HandleDeoxys(species, personality, isFrontPic, GetWindowTileDataAddress(windowId), FALSE))
        return 0xFFFF;

    LoadPicPaletteBySlot(species, otId, personality, paletteSlot, isTrainer);
    return 0;
}

u16 CreateTrainerCardSprite(u16 species, u32 otId, u32 personality, bool8 isFrontPic, u16 destX, u16 destY, u8 paletteSlot, u8 windowId, bool8 isTrainer)
{
    u8 *framePics;

    framePics = Alloc(4 * 0x800);
    if (framePics && !DecompressPic_HandleDeoxys(species, personality, isFrontPic, framePics, isTrainer))
    {
        BlitBitmapRectToWindow(windowId, framePics, 0, 0, 0x40, 0x40, destX, destY, 0x40, 0x40);
        LoadPicPaletteBySlot(species, otId, personality, paletteSlot, isTrainer);
        Free(framePics);
        return 0;
    }
    return 0xFFFF;
}

u16 CreateMonPicSprite(u16 species, u32 otId, u32 personality, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag, bool8 ignoreDeoxys)
{
    return CreatePicSprite(species, otId, personality, isFrontPic, x, y, paletteSlot, paletteTag, FALSE, ignoreDeoxys);
}

u16 CreateMonPicSprite_HandleDeoxys(u16 species, u32 otId, u32 personality, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag)
{
    return CreateMonPicSprite(species, otId, personality, isFrontPic, x, y, paletteSlot, paletteTag, FALSE);
}

u16 FreeAndDestroyMonPicSprite(u16 spriteId)
{
    return FreeAndDestroyPicSpriteInternal(spriteId);
}

u16 LoadMonPicInWindow(u16 species, u32 otId, u32 personality, bool8 isFrontPic, u8 paletteSlot, u8 windowId)
{
    return CreateTrainerCardSprite(species, otId, personality, isFrontPic, 0, 0, paletteSlot, windowId, FALSE);
}

u16 CreateTrainerCardMonIconSprite(u16 species, u32 otId, u32 personality, bool8 isFrontPic, u16 destX, u16 destY, u8 paletteSlot, u8 windowId)
{
    return CreateTrainerCardSprite(species, otId, personality, isFrontPic, destX, destY, paletteSlot, windowId, FALSE);
}

u16 CreateTrainerPicSprite(u16 species, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag)
{
    return CreatePicSprite_HandleDeoxys(species, 0, 0, isFrontPic, x, y, paletteSlot, paletteTag, TRUE);
}

u16 FreeAndDestroyTrainerPicSprite(u16 spriteId)
{
    return FreeAndDestroyPicSpriteInternal(spriteId);
}

u16 LoadTrainerPicInWindow(u16 species, bool8 isFrontPic, u8 paletteSlot, u8 windowId)
{
    return LoadPicSpriteInWindow(species, 0, 0, isFrontPic, paletteSlot, windowId, TRUE);
}

u16 CreateTrainerCardTrainerPicSprite(u16 species, bool8 isFrontPic, u16 destX, u16 destY, u8 paletteSlot, u8 windowId)
{
    return CreateTrainerCardSprite(species, 0, 0, isFrontPic, destX, destY, paletteSlot, windowId, TRUE);
}

u16 PlayerGenderToFrontTrainerPicId(u8 gender, bool8 getClass)
{
    if (getClass == TRUE)
    {
        if (gender != MALE)
            return gFacilityClassToPicIndex[FACILITY_CLASS_LEAF];
        else
            return gFacilityClassToPicIndex[FACILITY_CLASS_RED];
    }
    return gender;
}
