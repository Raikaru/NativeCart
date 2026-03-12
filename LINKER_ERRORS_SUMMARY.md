# Predicted Linker Errors - Pokemon FireRed GBA Decompilation Analysis

## Executive Summary

**Total Potential Undefined References: 1,319 symbols**

This analysis scanned 272 C files and identified symbols that are **called but never defined** in the source code. These will cause "undefined reference" linker errors.

---

## Critical Categories (Priority Order)

### 1. INTERRUPT HANDLERS (10 symbols) ⚠️ HIGH PRIORITY
Functions critical for GBA hardware operation:

- `GetIntroSpeechOfApproachingTrainer` - Called 3x in battle_setup.c
- `IntrEnable` - Called 2x in librfu_stwi.c (RFU/Wireless library)
- `SetFlashTimerIntr` - Called in main.c:397
- `intrCallback` - Called in field_effect.c:2845
- **Graphics data**: sControlsGuide_PikachuIntro_Background_Tiles, sPikachuIntro_Background_Tilemap, etc.

### 2. SOUND/MUSIC SYSTEM (10 symbols) ⚠️ HIGH PRIORITY
Core audio functions:

- `SetPokemonCryTone` - Called 5x (critical for Pokemon cries)
- `SoundMain` - Called in m4a.c:104 (main sound driver entry)
- `m4aSoundVSync` - Called in main.c:414 (audio sync with VBlank)
- **Graphics data**: sBorderBgMap, sScene2_GengarClose_Gfx, etc.

### 3. SAVE/FLASH SYSTEM (13 symbols) ⚠️ CRITICAL
Platform-specific GBA flash memory functions:

- `EraseFlashSector` - Called 4x in save.c
- `ProgramFlashByte` - Called 5x in save.c
- `ProgramFlashSectorAndVerify` - Called 2x in save.c
- `ReadFlash` - Called 3x in save.c
- `IdentifyFlash` - Called in load_save.c:47
- **Missing Mystery Gift functions**:
  - `GetSavedWonderCard` (5 calls)
  - `GetSavedWonderCardMetadata` (4 calls)
  - `GetSavedWonderNews` (4 calls)
  - `GetSavedWonderNewsMetadata` (6 calls)
  - `GetSavedRamScriptIfValid` (2 calls)

### 4. DMA FUNCTIONS (27 symbols) ⚠️ HIGH PRIORITY
Direct Memory Access operations:

- `DmaCopy16` / `DmaCopy32` - Called 22x and 3x
- `DmaFill16` / `DmaFill32` - Called 17x and 10x
- `DmaClear16` / `DmaClear32` / `DmaClearLarge16` - Called frequently
- `DmaSet` - Called 11x
- `DmaStop` - Called 23x

### 5. BATTLE SYSTEM (81 symbols)
Missing battle graphics and functions:

- `BattleAI_ChooseMoveOrAction` - AI opponent decision making
- `BattleAI_SetupAIData` - AI initialization
- `BattleSetup_ConfigureTrainerBattle` - Trainer battle setup
- `GetBattleInterfaceGfxPtr` - Called 20x (battle UI)
- **Missing terrain graphics**:
  - sBattleTerrainTiles_*, sBattleTerrainTilemap_*, sBattleTerrainPalette_* for all terrains
  - sBattleTerrainAnimTiles_*, sBattleTerrainAnimTilemap_*

### 6. SCRIPT SYSTEM (23 symbols)
Field script handling:

- `GetInteractedObjectEventScript` - Called 3x
- `GetInteractedBackgroundEventScript` - Called 3x
- `GetCoordEventScriptAtPosition` - Called 3x
- `ScriptReadByte` - Called 152x (heavily used!)
- `SetupNativeScript` - Called 19x

### 7. SPRITE SYSTEM (41 symbols)
Object and Pokemon sprites:

- `CreateInvisibleSprite` / `CreateInvisibleSpriteWithCallback`
- `GetMonFrontSpritePal` / `GetMonSpritePalFromSpeciesAndPersonality`
- `CreateMonSpritesGfxManager`
- Various `s*SpriteGfx` and `s*SpritePalette` data arrays

### 8. BACKGROUND SYSTEM (36 symbols)
Background graphics and tilemaps:

- `GetBgTilemapBuffer` - Called 43x
- Various `sBg*_Gfx`, `sBg*_Pal`, `sBg*_Tilemap` data arrays

### 9. GLOBAL VARIABLES / CALLBACKS (30 symbols)
State machine and callback functions:

- `gBattleMainFunc` - Main battle state function
- `gFieldCallback` / `gFieldCallback2` - Field state callbacks
- `gItemUseCB` - Item usage callback
- `gLinkCallback` - Link connection callback
- `gAnimScriptCallback` - Animation callback (9 calls)

---

## Most Frequently Called Missing Functions

1. **ScriptReadByte** - 152 calls (script system)
2. **AllocZeroed** - 171 calls (memory allocation)
3. **Alloc** - 106 calls (memory allocation)
4. **AddTextPrinterParameterized** - 91 calls (text rendering)
5. **GetBgTilemapBuffer** - 43 calls (background system)
6. **func** - 38 calls (generic callbacks)
7. **RecordAbilityBattle** - 31 calls (battle system)
8. **DmaCopy16** - 22 calls (DMA)
9. **DmaStop** - 23 calls (DMA)
10. **GetBattleInterfaceGfxPtr** - 20 calls (battle UI)

---

## Key Files Requiring Missing Symbols

| File | Missing Symbols Count |
|------|----------------------|
| party_menu.c | ~40+ missing functions |
| battle_script_commands.c | ~30+ missing functions |
| scrcmd.c | ~25+ missing functions |
| battle_anim_mons.c | ~20+ missing functions |
| start_menu.c | ~15+ missing functions |
| item_use.c | ~15+ missing functions |

---

## Recommendations

### Immediate Actions:

1. **Save/Flash System**: Implement the flash memory functions (`EraseFlashSector`, `ProgramFlashByte`, `ReadFlash`, etc.) - these are platform-specific GBA functions

2. **DMA Functions**: Implement DMA wrapper functions for memory transfers

3. **Graphics Data**: Many `s*` prefixed symbols are likely static graphics data arrays (tilemaps, palettes, sprites) that should be defined in separate graphics files

4. **Script System**: Implement `ScriptReadByte` and related script parsing functions

5. **Sound System**: Connect `m4aSoundVSync` and `SetPokemonCryTone` to your audio backend

### Secondary Priority:

6. Battle terrain graphics (81 symbols)
7. Sprite and background graphics data
8. Mystery Gift save/load functions
9. Callback and function pointer variables

---

## Files Generated

- `linker_error_predictor.py` - Analysis script
- `linker_errors_report.json` - Full machine-readable report
- This document - Human-readable summary

## Next Steps

1. Run a test build to confirm which symbols actually cause linker errors
2. Implement stub functions for critical missing symbols
3. Create graphics data files for missing static arrays
4. Connect GBA-specific functions (flash, DMA, interrupts) to your GDExtension backend

---

*Analysis completed: Generated by linker_error_predictor.py*
