#ifndef GUARD_DATA_H
#define GUARD_DATA_H

#include "global.h"

#define SPECIES_SHINY_TAG 500
#define TRAINER_ENCOUNTER_MUSIC(trainer)((gTrainers[trainer].encounterMusic_gender & 0x7F))

struct MonCoords
{
    // This would use a bitfield, but some function
    // uses it as a u8 and casting won't match.
    u8 size; // u8 width:4, height:4;
    u8 y_offset;
};

#define MON_COORDS_SIZE(width, height)(DIV_ROUND_UP(width, 8) << 4 | DIV_ROUND_UP(height, 8))
#define GET_MON_COORDS_WIDTH(size)((size >> 4) * 8)
#define GET_MON_COORDS_HEIGHT(size)((size & 0xF) * 8)

extern const u8 gSpeciesNames[][POKEMON_NAME_LENGTH + 1];
extern const u8 gMoveNames_Compiled[][MOVE_NAME_LENGTH + 1];
#ifdef PORTABLE
const u8 *FireredMoveNamesBytes(void);
#define gMoveNames ((const u8 (*)[MOVE_NAME_LENGTH + 1])FireredMoveNamesBytes())
#else
#define gMoveNames gMoveNames_Compiled
#endif

extern const u8 gTrainerClassNames[][13];

extern const struct MonCoords gMonFrontPicCoords_Compiled[];
extern const struct CompressedSpriteSheet gMonFrontPicTable[];
extern const struct MonCoords gMonBackPicCoords_Compiled[];
extern const struct CompressedSpriteSheet gMonBackPicTable[];
extern const struct CompressedSpritePalette gMonPaletteTable[];
extern const struct CompressedSpritePalette gMonShinyPaletteTable[];
extern const union AnimCmd *const *const gTrainerFrontAnimsPtrTable[];
extern const struct MonCoords gTrainerFrontPicCoords[];
extern const struct CompressedSpriteSheet gTrainerFrontPicTable[];
extern const struct CompressedSpriteSheet gTrainerBackPicTable[];
extern const struct CompressedSpritePalette gTrainerFrontPicPaletteTable[];
extern const union AnimCmd *const *const gTrainerBackAnimsPtrTable[];
extern const struct MonCoords gTrainerBackPicCoords[];
extern const struct CompressedSpritePalette gTrainerBackPicPaletteTable[];

extern const struct CompressedSpriteSheet gSpriteSheet_EnemyShadow;
extern const struct SpriteTemplate gSpriteTemplate_EnemyShadow;

extern const u8 gEnemyMonElevation_Compiled[NUM_SPECIES];
#ifdef PORTABLE
/* NULL => use compiled tables (see macros below). */
extern const struct MonCoords *gMonFrontPicCoordsActive;
extern const struct MonCoords *gMonBackPicCoordsActive;
extern const u8 *gEnemyMonElevationActive;
#define gMonFrontPicCoords  ((gMonFrontPicCoordsActive) != NULL ? (gMonFrontPicCoordsActive) : (gMonFrontPicCoords_Compiled))
#define gMonBackPicCoords   ((gMonBackPicCoordsActive) != NULL ? (gMonBackPicCoordsActive) : (gMonBackPicCoords_Compiled))
#define gEnemyMonElevation  ((gEnemyMonElevationActive) != NULL ? (gEnemyMonElevationActive) : (gEnemyMonElevation_Compiled))
#else
#define gMonFrontPicCoords  gMonFrontPicCoords_Compiled
#define gMonBackPicCoords   gMonBackPicCoords_Compiled
#define gEnemyMonElevation  gEnemyMonElevation_Compiled
#endif

extern const u8 *const gBattleAnims_General[];
extern const u8 *const gBattleAnims_Special[];

extern const union AnimCmd *const gAnims_MonPic[];
extern const union AffineAnimCmd *const gAffineAnims_BattleSpritePlayerSide[];
extern const union AffineAnimCmd *const gAffineAnims_BattleSpriteOpponentSide[];
extern const struct SpriteFrameImage gBattlerPicTable_PlayerLeft[];
extern const struct SpriteFrameImage gBattlerPicTable_OpponentLeft[];
extern const struct SpriteFrameImage gBattlerPicTable_PlayerRight[];
extern const struct SpriteFrameImage gBattlerPicTable_OpponentRight[];
extern const struct SpriteFrameImage gTrainerBackPicTable_Red[];
extern const struct SpriteFrameImage gTrainerBackPicTable_Leaf[];
extern const struct SpriteFrameImage gTrainerBackPicTable_Pokedude[];
extern const struct SpriteFrameImage gTrainerBackPicTable_OldMan[];
extern const struct SpriteFrameImage gTrainerBackPicTable_RSBrendan[];
extern const struct SpriteFrameImage gTrainerBackPicTable_RSMay[];

#endif // GUARD_DATA_H
