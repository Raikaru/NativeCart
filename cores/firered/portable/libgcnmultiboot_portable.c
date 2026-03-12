#include "global.h"
#include "libgcnmultiboot.h"

void GameCubeMultiBoot_Main(struct GcmbStruct *pStruct)
{
    (void)pStruct;
}

void GameCubeMultiBoot_ExecuteProgram(struct GcmbStruct *pStruct)
{
    (void)pStruct;
}

void GameCubeMultiBoot_Init(struct GcmbStruct *pStruct)
{
    if (pStruct != NULL)
        pStruct->gcmb_field_2 = 0;
}

void GameCubeMultiBoot_HandleSerialInterrupt(struct GcmbStruct *pStruct)
{
    (void)pStruct;
}

void GameCubeMultiBoot_Quit(void)
{
}
