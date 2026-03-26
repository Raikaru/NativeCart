/* GetEggSpecies equivalent: thumb agbcc ICEs on this loop (stor-layout.c:203). Built with agbcc_arm (see Makefile). */

#include "global.h"

#ifndef PORTABLE
extern const struct Evolution gEvolutionTable[][EVOS_PER_MON];
#endif

u16 Daycare_GetEggSpecies(u16 species)
{
#ifndef PORTABLE
    int i, j, k;
    bool8 found;

    for (i = 0; i < EVOS_PER_MON; i++)
    {
        found = FALSE;
        for (j = 1; j < NUM_SPECIES; j++)
        {
            for (k = 0; k < EVOS_PER_MON; k++)
            {
                if (gEvolutionTable[j][k].targetSpecies == species)
                {
                    species = (u16)j;
                    found = TRUE;
                    break;
                }
            }

            if (found)
                break;
        }

        if (j == NUM_SPECIES)
            break;
    }
#endif
    return species;
}
