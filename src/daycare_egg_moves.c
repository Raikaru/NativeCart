/* Egg move / hatch moveset logic split from daycare.c (reduces agbcc stor-layout ICE). */

#include "global.h"
#include "party_menu.h"
#include "random.h"
#include "constants/daycare.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "daycare.h"

#include "data/pokemon/egg_moves.h"
#ifdef PORTABLE
#include "portable/firered_portable_rom_egg_moves_table.h"
#endif

EWRAM_DATA static u16 sHatchedEggLevelUpMoves[EGG_LVL_UP_MOVES_ARRAY_COUNT] = {0};
EWRAM_DATA static u16 sHatchedEggFatherMoves[4] = {0};
EWRAM_DATA static u16 sHatchedEggFinalMoves[4] = {0};
EWRAM_DATA static u16 sHatchedEggEggMoves[EGG_MOVES_ARRAY_COUNT] = {0};
EWRAM_DATA static u16 sHatchedEggMotherMoves[4] = {0};

static u8 GetEggMoves(struct Pokemon *pokemon, u16 *eggMoves)
{
    u16 eggMoveIdx;
    u16 numEggMoves;
    u16 species;
    u16 i;

    numEggMoves = 0;
    eggMoveIdx = 0;
    species = GetMonData(pokemon, MON_DATA_SPECIES);
#ifdef PORTABLE
    for (i = 0; i < FireredEggMovesTableWordCount() - 1; i++)
    {
        if (FireredEggMovesTable()[i] == species + EGG_MOVES_SPECIES_OFFSET)
        {
            eggMoveIdx = i + 1;
            break;
        }
    }

    for (i = 0; i < EGG_MOVES_ARRAY_COUNT; i++)
    {
        if (FireredEggMovesTable()[eggMoveIdx + i] > EGG_MOVES_SPECIES_OFFSET)
        {
            break;
        }

        eggMoves[i] = FireredEggMovesTable()[eggMoveIdx + i];
        numEggMoves++;
    }
#else
    for (i = 0; i < NELEMS(gEggMoves) - 1; i++)
    {
        if (gEggMoves[i] == species + EGG_MOVES_SPECIES_OFFSET)
        {
            eggMoveIdx = i + 1;
            break;
        }
    }

    for (i = 0; i < EGG_MOVES_ARRAY_COUNT; i++)
    {
        if (gEggMoves[eggMoveIdx + i] > EGG_MOVES_SPECIES_OFFSET)
        {
            break;
        }

        eggMoves[i] = gEggMoves[eggMoveIdx + i];
        numEggMoves++;
    }
#endif

    return numEggMoves;
}

void Daycare_BuildEggMoveset(struct Pokemon *egg, struct BoxPokemon *father, struct BoxPokemon *mother)
{
    u16 numSharedParentMoves;
    u32 numLevelUpMoves;
    u16 numEggMoves;
    u16 i, j;

    numSharedParentMoves = 0;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        sHatchedEggMotherMoves[i] = MOVE_NONE;
        sHatchedEggFatherMoves[i] = MOVE_NONE;
        sHatchedEggFinalMoves[i] = MOVE_NONE;
    }
    for (i = 0; i < EGG_MOVES_ARRAY_COUNT; i++)
        sHatchedEggEggMoves[i] = MOVE_NONE;
    for (i = 0; i < EGG_LVL_UP_MOVES_ARRAY_COUNT; i++)
        sHatchedEggLevelUpMoves[i] = MOVE_NONE;

    numLevelUpMoves = GetLevelUpMovesBySpecies(GetMonData(egg, MON_DATA_SPECIES), sHatchedEggLevelUpMoves);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        sHatchedEggFatherMoves[i] = GetBoxMonData(father, MON_DATA_MOVE1 + i);
        sHatchedEggMotherMoves[i] = GetBoxMonData(mother, MON_DATA_MOVE1 + i);
    }

    numEggMoves = GetEggMoves(egg, sHatchedEggEggMoves);

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (sHatchedEggFatherMoves[i] != MOVE_NONE)
        {
            for (j = 0; j < numEggMoves; j++)
            {
                if (sHatchedEggFatherMoves[i] == sHatchedEggEggMoves[j])
                {
                    if (GiveMoveToMon(egg, sHatchedEggFatherMoves[i]) == MON_HAS_MAX_MOVES)
                        DeleteFirstMoveAndGiveMoveToMon(egg, sHatchedEggFatherMoves[i]);
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (sHatchedEggFatherMoves[i] != MOVE_NONE)
        {
            for (j = 0; j < NUM_TECHNICAL_MACHINES + NUM_HIDDEN_MACHINES; j++)
            {
                if (sHatchedEggFatherMoves[i] == ItemIdToBattleMoveId(ITEM_TM01/*_FOCUS_PUNCH*/ + j) && CanMonLearnTMHM(egg, j))
                {
                    if (GiveMoveToMon(egg, sHatchedEggFatherMoves[i]) == MON_HAS_MAX_MOVES)
                        DeleteFirstMoveAndGiveMoveToMon(egg, sHatchedEggFatherMoves[i]);
                }
            }
        }
    }
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (sHatchedEggFatherMoves[i] == MOVE_NONE)
            break;
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            if (sHatchedEggFatherMoves[i] == sHatchedEggMotherMoves[j] && sHatchedEggFatherMoves[i] != MOVE_NONE)
                sHatchedEggFinalMoves[numSharedParentMoves++] = sHatchedEggFatherMoves[i];
        }
    }

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (sHatchedEggFinalMoves[i] == MOVE_NONE)
            break;
        for (j = 0; j < numLevelUpMoves; j++)
        {
            if (sHatchedEggLevelUpMoves[j] != MOVE_NONE && sHatchedEggFinalMoves[i] == sHatchedEggLevelUpMoves[j])
            {
                if (GiveMoveToMon(egg, sHatchedEggFinalMoves[i]) == MON_HAS_MAX_MOVES)
                    DeleteFirstMoveAndGiveMoveToMon(egg, sHatchedEggFinalMoves[i]);
                break;
            }
        }
    }
}
