#include "global.h"
#include "battle.h"
#include "event_object_lock.h"
#include "field_control_avatar.h"
#include "field_player_avatar.h"
#include "field_effect.h"
#include "fieldmap.h"
#include "global.fieldmap.h"
#include "item_menu.h"
#include "item_use.h"
#include "metatile_behavior.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "poke_radar.h"
#include "random.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "constants/battle.h"
#include "constants/field_effects.h"
#include "constants/songs.h"
#include "constants/species.h"

struct PokeRadarChain gPokeRadarChain;

static const u8 sText_GrassyPatchRemainedSilent[] = _("The grassy patch remained\nsilent...{PAUSE_UNTIL_PRESS}");
static const u8 sText_StepsUntilCharged[] = _("The battery has run dry!\n{STR_VAR_1} steps until fully charged.{PAUSE_UNTIL_PRESS}");
static const u8 sText_StepUntilCharged[] = _("The battery has run dry!\n1 step until fully charged.{PAUSE_UNTIL_PRESS}");

static void WaitForShakingPokeRadarGrass(u8 taskId);
static void FinishPokeRadar(u8 taskId);

static bool8 IsValidPokeRadarMetatile(s16 x, s16 y)
{
    u16 tileBehavior;
    s16 gridX, gridY;
    
    gridX = x - 7;
    gridY = y - 7;
    
    if (gridX < 0 || gridY < 0 || gridX >= gMapHeader.mapLayout->width || gridY >= gMapHeader.mapLayout->height)
        return FALSE;

    tileBehavior = MapGridGetMetatileBehaviorAt(x, y);
    if (!MetatileBehavior_IsTallGrass(tileBehavior))
        return FALSE;

    return TRUE;
}

bool8 CanUsePokeRadar(u8 taskId)
{
    s16 x, y;
    
    PlayerGetDestCoords(&x, &y);
    if (!IsValidPokeRadarMetatile(x, y))
    {
        return FALSE;
    }

    return TRUE;
}

static const u8 sNumPatchesInRing[] = {32, 24, 16, 8};

static const s8 sFourthRingCoords[][2] = {
    {-4, -4},{-4, -3},{-4, -2},{-4, -1},{-4,  0},{-4,  1},{-4,  2},{-4,  3},{-4,  4},
    {-3, -4},                                                               {-3,  4},
    {-2, -4},                                                               {-2,  4},
    {-1, -4},                                                               {-1,  4},
    { 0, -4},                                                               { 0,  4},
    { 1, -4},                                                               { 1,  4},
    { 2, -4},                                                               { 2,  4},
    { 3, -4},                                                               { 3,  4},
    { 4, -4},{ 4, -3},{ 4, -2},{ 4, -1},{ 4,  0},{ 4,  1},{ 4,  2},{ 4,  3},{ 4,  4},
};

static const s8 sThirdRingCoords[][2] = {
    {-3, -3},{-3, -2},{-3, -1},{-3,  0},{-3,  1},{-3,  2},{-3,  3},
    {-2, -3},                                             {-2,  3},
    {-1, -3},                                             {-1,  3},
    { 0, -3},                                             { 0,  3},
    { 1, -3},                                             { 1,  3},
    { 2, -3},                                             { 2,  3},
    { 3, -3},{ 3, -2},{ 3, -1},{ 3,  0},{ 3,  1},{ 3,  2},{ 3,  3},
};

static const s8 sSecondRingCoords[][2] = {
    {-2, -2},{-2, -1},{-2,  0},{-2,  1},{-2,  2},
    {-1, -2},                           {-1,  2},
    { 0, -2},                           { 0,  2},
    { 1, -2},                           { 1,  2},
    { 2, -2},{ 2, -1},{ 2,  0},{ 2,  1},{ 2,  2},
};

static const s8 sFirstRingCoords[][2] = {
    {-1, -1},{-1,  0},{-1,  1},
    { 0, -1},         { 0,  1},
    { 1, -1},{ 1,  0},{ 1,  1},
};

typedef const s8 (*RingCoords)[2];

static const RingCoords sPokeRadarRingCoords[] =
{
    sFourthRingCoords,
    sThirdRingCoords,
    sSecondRingCoords,
    sFirstRingCoords
};

bool8 ChoosePokeRadarShakeCoords(s16 baseX, s16 baseY)
{
    u32 i;
    bool8 valid = FALSE;
    
    for (i = 0; i < NUM_POKE_RADAR_GRASS_PATCHES; i++)
    {
        u16 index = Random() % sNumPatchesInRing[i];
        gPokeRadarChain.grassPatches[i].x = baseX + sPokeRadarRingCoords[i][index][0];
        gPokeRadarChain.grassPatches[i].y = baseY + sPokeRadarRingCoords[i][index][1];
        gPokeRadarChain.grassPatches[i].active = IsValidPokeRadarMetatile(gPokeRadarChain.grassPatches[i].x, gPokeRadarChain.grassPatches[i].y);
        
        if (gPokeRadarChain.grassPatches[i].active)
            valid = TRUE;
    }
    
    return valid;
}

static const u8 sPatchRates[] = {88, 68, 48, 28};

static bool8 CheckPatchContinuesChain(u8 patchIndex, u8 increasedRates)
{
    const u8 *rates = sPatchRates;
    return increasedRates ? (Random() % 100) < (rates[patchIndex] + 10) : (Random() % 100) < rates[patchIndex];
}

static u8 CheckShinyPatch(u8 chain)
{
    u32 shinyChance;
    
    if (chain == 0)
        return 0;

    shinyChance = 8200 - chain * 200; // max chain of 40 comes from 40 * 200 = 8000; 8200 - 8000 = 200
    if (shinyChance < 200)
        shinyChance = 200;
    return ((Random() / ((0xFFFF / shinyChance) + 1)) == 0);
}

static void PrepGrassPatchChainData(void)
{
    u32 i;
    for (i = 0; i < NUM_POKE_RADAR_GRASS_PATCHES; i++)
    {
        if (gPokeRadarChain.grassPatches[i].active)
        {
            gPokeRadarChain.grassPatches[i].continueChain = CheckPatchContinuesChain(i, gPokeRadarChain.increasedRates);
            gPokeRadarChain.increasedRates = 0;
            if (gPokeRadarChain.grassPatches[i].continueChain)
            {
                gPokeRadarChain.grassPatches[i].patchType = gPokeRadarChain.chain > 0 ? gPokeRadarChain.patchType : (Random() & 1);
                gPokeRadarChain.grassPatches[i].isShiny = CheckShinyPatch(gPokeRadarChain.chain);
            }
            else
            {
                gPokeRadarChain.grassPatches[i].patchType = Random() & 1;
                gPokeRadarChain.grassPatches[i].isShiny = 0;
            }
        }
    }
}

void TrySetPokeRadarPatchCoords(void)
{
    if (gPokeRadarChain.active)
    {
        s16 x, y;
        PlayerGetDestCoords(&x, &y);
        if (!ChoosePokeRadarShakeCoords(x, y))
            BreakPokeRadarChain();
    }
}

void StartPokeRadarGrassShake(void)
{
    u32 i;
    struct ObjectEvent *playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];

    PlayBGM(MUS_POKE_RADAR);

    DisableWildPokemonImmunity();
    PrepGrassPatchChainData();
    
    for (i = 0; i < NUM_POKE_RADAR_GRASS_PATCHES; i++)
    {
        if (IsValidPokeRadarMetatile(gPokeRadarChain.grassPatches[i].x, gPokeRadarChain.grassPatches[i].y))
        {
            gFieldEffectArguments[0] = gPokeRadarChain.grassPatches[i].x;
            gFieldEffectArguments[1] = gPokeRadarChain.grassPatches[i].y;
            gFieldEffectArguments[2] = playerObj->previousElevation;
            gFieldEffectArguments[3] = 2;
            gFieldEffectArguments[4] = playerObj->localId << 8 | playerObj->mapNum;
            gFieldEffectArguments[5] = playerObj->mapGroup;
            gFieldEffectArguments[6] = (u8)gSaveBlock1Ptr->location.mapNum << 8 | (u8)gSaveBlock1Ptr->location.mapGroup;
            gFieldEffectArguments[7] = 0;
                
            if (gPokeRadarChain.grassPatches[i].patchType == 0)
                FieldEffectStart(FLDEFF_TALL_GRASS);
            else
                FieldEffectStart(FLDEFF_POKE_RADAR_GRASS);
            if (gPokeRadarChain.grassPatches[i].isShiny)
                FieldEffectStart(FLDEFF_POKE_RADAR_SPARKLE);
        }
    }
}

#define tWaitDuration data[0]

void Task_StartPokeRadarGrassShake(u8 taskId)
{
    ScriptContext2_Enable();
    gPlayerAvatar.preventStep = TRUE;
    gPokeRadarChain.stepsUntilCharged = POKE_RADAR_STEPS_TO_CHARGE;
    gPokeRadarChain.originX = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.x;
    gPokeRadarChain.originY = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.y;
    gPokeRadarChain.mapGroup = gSaveBlock1Ptr->location.mapGroup;
    gPokeRadarChain.mapNum = gSaveBlock1Ptr->location.mapNum;
    StartPokeRadarGrassShake();
    gTasks[taskId].tWaitDuration = 60;
    gTasks[taskId].func = WaitForShakingPokeRadarGrass;
}

void ItemUseOnFieldCB_PokeRadar(u8 taskId)
{
    struct ObjectEvent *playerObj;
    
    if (gPokeRadarChain.stepsUntilCharged > 0)
    {
        if (gPokeRadarChain.stepsUntilCharged == 1)
            DisplayItemMessageOnField(taskId, sText_StepUntilCharged, Task_CloseCantUseKeyItemMessage);
        else
        {
            ConvertIntToDecimalStringN(gStringVar1, gPokeRadarChain.stepsUntilCharged, 0, 2);
            StringExpandPlaceholders(gStringVar4, sText_StepsUntilCharged);
            DisplayItemMessageOnField(taskId, gStringVar4, Task_CloseCantUseKeyItemMessage);
        }
        
        return;
    }
    
    playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    
    if (ChoosePokeRadarShakeCoords(playerObj->currentCoords.x, playerObj->currentCoords.y))
    {
        gPokeRadarChain.active = 1;
        gPokeRadarChain.stepsUntilCharged = POKE_RADAR_STEPS_TO_CHARGE;
        Task_StartPokeRadarGrassShake(taskId);
    }
    else
    {
        BreakPokeRadarChain();
        DisplayItemMessageOnField(taskId, sText_GrassyPatchRemainedSilent, Task_CloseCantUseKeyItemMessage);
    }
}

static void WaitForShakingPokeRadarGrass(u8 taskId)
{
    if (--gTasks[taskId].tWaitDuration < 0)
        FinishPokeRadar(taskId);
}

static void FinishPokeRadar(u8 taskId)
{
    ScriptUnfreezeObjectEvents();
    gPlayerAvatar.preventStep = FALSE;
    ScriptContext2_Disable();
    DestroyTask(taskId);
}

void BreakPokeRadarChain(void)
{
    u32 i;

    Overworld_PlaySpecialMapMusic();

    gPokeRadarChain.chain = 0;
    gPokeRadarChain.species = SPECIES_NONE;
    gPokeRadarChain.level = 0;
    gPokeRadarChain.patchType = 0;
    gPokeRadarChain.active = 0;
    gPokeRadarChain.originX = 0;
    gPokeRadarChain.originY = 0;
    gPokeRadarChain.mapGroup = 0;
    gPokeRadarChain.mapNum = 0;
    
    for (i = 0; i < NUM_POKE_RADAR_GRASS_PATCHES; i++)
    {
        gPokeRadarChain.grassPatches[i].x = 0;
        gPokeRadarChain.grassPatches[i].y = 0;
        gPokeRadarChain.grassPatches[i].patchType = 0;
        gPokeRadarChain.grassPatches[i].active = 0;
        gPokeRadarChain.grassPatches[i].continueChain = 0;
        gPokeRadarChain.grassPatches[i].isShiny = 0;
    }
}

void IncrementPokeRadarChain(void)
{
    if (++gPokeRadarChain.chain > 254)
        gPokeRadarChain.chain = 254;
}

void SetPokeRadarPokemon(u16 species, u8 level)
{
    gPokeRadarChain.species = species;
    gPokeRadarChain.level = level;
}

void UpdatePokeRadarAfterWildBattle(u8 battleOutcome)
{
    if (gPokeRadarChain.active)
    {
        switch (battleOutcome)
        {
        case B_OUTCOME_LOST:
        case B_OUTCOME_DREW:
        case B_OUTCOME_RAN:
        case B_OUTCOME_PLAYER_TELEPORTED:
        case B_OUTCOME_MON_FLED:
        case B_OUTCOME_NO_SAFARI_BALLS:
        case B_OUTCOME_MON_TELEPORTED:
            BreakPokeRadarChain();
            break;
        case B_OUTCOME_CAUGHT:
            gPokeRadarChain.increasedRates = 1;
            break;
        case B_OUTCOME_WON:
            gPokeRadarChain.increasedRates = 0;
            break;
        }
    }
}

void InitNewPokeRadarChain(u16 species, u8 level, u8 patchType)
{
    SetPokeRadarPokemon(species, level);
    gPokeRadarChain.patchType = patchType;
    gPokeRadarChain.chain = 1;
}

void ChargePokeRadar(void)
{
    s16 xDiff, yDiff;
    if (gPokeRadarChain.stepsUntilCharged > 0)
        gPokeRadarChain.stepsUntilCharged--;

    if (gPokeRadarChain.active)
    {
        if (gSaveBlock1Ptr->location.mapGroup != gPokeRadarChain.mapGroup || gSaveBlock1Ptr->location.mapNum != gPokeRadarChain.mapNum)
        {
            BreakPokeRadarChain();
            return;
        }
        
        xDiff = abs(gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.x - gPokeRadarChain.originX);
        yDiff = abs(gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.y - gPokeRadarChain.originY);
        if (xDiff > 13 || yDiff > 13)
        {
            BreakPokeRadarChain();
            return;
        }
    }
}
