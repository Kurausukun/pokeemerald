	.include "asm/macros.inc"
	.include "constants/constants.inc"

	.section script_data

	.align 2
gFieldEffectScriptPointers:: @ 82DB9D4
	.int gFieldEffectScript_ExclamationMarkIcon1      @ FLDEFF_EXCLAMATION_MARK_ICON
	.int gFieldEffectScript_UseCutOnTallGrass         @ FLDEFF_USE_CUT_ON_GRASS
	.int gFieldEffectScript_UseCutOnTree              @ FLDEFF_USE_CUT_ON_TREE
	.int gFieldEffectScript_Shadow                    @ FLDEFF_SHADOW
	.int gFieldEffectScript_TallGrass                 @ FLDEFF_TALL_GRASS
	.int gFieldEffectScript_Ripple                    @ FLDEFF_RIPPLE
	.int gFieldEffectScript_FieldMoveShowMon          @ FLDEFF_FIELD_MOVE_SHOW_MON
	.int gFieldEffectScript_Ash                       @ FLDEFF_ASH
	.int gFieldEffectScript_SurfBlob                  @ FLDEFF_SURF_BLOB
	.int gFieldEffectScript_UseSurf                   @ FLDEFF_USE_SURF
	.int gFieldEffectScript_GroundImpactDust          @ FLDEFF_DUST
	.int gFieldEffectScript_UseSecretPowerCave        @ FLDEFF_USE_SECRET_POWER_CAVE
	.int gFieldEffectScript_JumpTallGrass             @ FLDEFF_JUMP_TALL_GRASS
	.int gFieldEffectScript_SandFootprints            @ FLDEFF_SAND_FOOTPRINTS
	.int gFieldEffectScript_JumpBigSplash             @ FLDEFF_JUMP_BIG_SPLASH
	.int gFieldEffectScript_Splash                    @ FLDEFF_SPLASH
	.int gFieldEffectScript_JumpSmallSplash           @ FLDEFF_JUMP_SMALL_SPLASH
	.int gFieldEffectScript_LongGrass                 @ FLDEFF_LONG_GRASS
	.int gFieldEffectScript_JumpLongGrass             @ FLDEFF_JUMP_LONG_GRASS
	.int gFieldEffectScript_UnusedGrass               @ FLDEFF_UNUSED_GRASS
	.int gFieldEffectScript_UnusedGrass2              @ FLDEFF_UNUSED_GRASS_2
	.int gFieldEffectScript_UnusedSand                @ FLDEFF_UNUSED_SAND
	.int gFieldEffectScript_WaterSurfacing            @ FLDEFF_WATER_SURFACING
	.int gFieldEffectScript_BerryTreeGrowthSparkle    @ FLDEFF_BERRY_TREE_GROWTH_SPARKLE
	.int gFieldEffectScript_DeepSandFootprints        @ FLDEFF_DEEP_SAND_FOOTPRINTS
	.int gFieldEffectScript_PokeCenterHeal            @ FLDEFF_POKECENTER_HEAL
	.int gFieldEffectScript_UseSecretPowerTree        @ FLDEFF_USE_SECRET_POWER_TREE
	.int gFieldEffectScript_UseSecretPowerShrub       @ FLDEFF_USE_SECRET_POWER_SHRUB
	.int gFieldEffectScript_TreeDisguise              @ FLDEFF_TREE_DISGUISE
	.int gFieldEffectScript_MountainDisguise          @ FLDEFF_MOUNTAIN_DISGUISE
	.int gFieldEffectScript_NPCUseFly                 @ FLDEFF_NPCFLY_OUT
	.int gFieldEffectScript_UseFly                    @ FLDEFF_USE_FLY
	.int gFieldEffectScript_FlyIn                     @ FLDEFF_FLY_IN
	.int gFieldEffectScript_QuestionMarkIcon          @ FLDEFF_QUESTION_MARK_ICON
	.int gFieldEffectScript_FeetInFlowingWater        @ FLDEFF_FEET_IN_FLOWING_WATER
	.int gFieldEffectScript_BikeTireTracks            @ FLDEFF_BIKE_TIRE_TRACKS
	.int gFieldEffectScript_SandDisguisePlaceholder   @ FLDEFF_SAND_DISGUISE
	.int gFieldEffectScript_UseRockSmash              @ FLDEFF_USE_ROCK_SMASH
	.int gFieldEffectScript_UseDig                    @ FLDEFF_USE_DIG
	.int gFieldEffectScript_SandPile                  @ FLDEFF_SAND_PILE
	.int gFieldEffectScript_UseStrength               @ FLDEFF_USE_STRENGTH
	.int gFieldEffectScript_ShortGrass                @ FLDEFF_SHORT_GRASS
	.int gFieldEffectScript_HotSpringsWater           @ FLDEFF_HOT_SPRINGS_WATER
	.int gFieldEffectScript_UseWaterfall              @ FLDEFF_USE_WATERFALL
	.int gFieldEffectScript_UseDive                   @ FLDEFF_USE_DIVE
	.int gFieldEffectScript_Pokeball                  @ FLDEFF_POKEBALL
	.int gFieldEffectScript_HeartIcon                 @ FLDEFF_HEART_ICON
	.int gFieldEffectScript_Nop47                     @ FLDEFF_NOP_47
	.int gFieldEffectScript_Nop48                     @ FLDEFF_NOP_48
	.int gFieldEffectScript_AshPuff                   @ FLDEFF_ASH_PUFF
	.int gFieldEffectScript_AshLaunch                 @ FLDEFF_ASH_LAUNCH
	.int gFieldEffectScript_SweetScent                @ FLDEFF_SWEET_SCENT
	.int gFieldEffectScript_SandPillar                @ FLDEFF_SAND_PILLAR
	.int gFieldEffectScript_Bubbles                   @ FLDEFF_BUBBLES
	.int gFieldEffectScript_Sparkle                   @ FLDEFF_SPARKLE
	.int gFieldEffectScript_ShowSecretPowerCave       @ FLDEFF_SECRET_POWER_CAVE
	.int gFieldEffectScript_ShowSecretPowerTree       @ FLDEFF_SECRET_POWER_TREE
	.int gFieldEffectScript_ShowSecretPowerShrub      @ FLDEFF_SECRET_POWER_SHRUB
	.int gFieldEffectScript_ShowCutGrass              @ FLDEFF_CUT_GRASS
	.int gFieldEffectScript_FieldMoveShowMonInit      @ FLDEFF_FIELD_MOVE_SHOW_MON_INIT
	.int gFieldEffectScript_UsePuzzleEffect           @ FLDEFF_USE_TOMB_PUZZLE_EFFECT
	.int gFieldEffectScript_SecretBaseBootPC          @ FLDEFF_PCTURN_ON
	.int gFieldEffectScript_HallOfFameRecord          @ FLDEFF_HALL_OF_FAME_RECORD
	.int gFieldEffectScript_UseTeleport               @ FLDEFF_USE_TELEPORT
	.int gFieldEffectScript_RayquazaSpotlight         @ FLDEFF_RAYQUAZA_SPOTLIGHT
	.int gFieldEffectScript_DestroyDeoxysRock         @ FLDEFF_DESTROY_DEOXYS_ROCK
	.int gFieldEffectScript_MoveDeoxysRock            @ FLDEFF_MOVE_DEOXYS_ROCK

gFieldEffectScript_ExclamationMarkIcon1:: @ 82DBAE0
	field_eff_callnative FldEff_ExclamationMarkIcon
	field_eff_end

gFieldEffectScript_UseCutOnTallGrass:: @ 82DBAE6
	field_eff_callnative FldEff_UseCutOnGrass
	field_eff_end

gFieldEffectScript_UseCutOnTree:: @ 82DBAEC
	field_eff_callnative FldEff_UseCutOnTree
	field_eff_end

gFieldEffectScript_Shadow:: @ 82DBAF2
	field_eff_callnative FldEff_Shadow
	field_eff_end

gFieldEffectScript_TallGrass:: @ 82DBAF8
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_TallGrass
	field_eff_end

gFieldEffectScript_Ripple:: @ 82DBB02
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_Ripple
	field_eff_end

gFieldEffectScript_FieldMoveShowMon:: @ 82DBB0C
	field_eff_callnative FldEff_FieldMoveShowMon
	field_eff_end

gFieldEffectScript_Ash:: @ 82DBB12
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_Ash
	field_eff_end

gFieldEffectScript_SurfBlob:: @ 82DBB1C
	field_eff_callnative FldEff_SurfBlob
	field_eff_end

gFieldEffectScript_UseSurf:: @ 82DBB22
	field_eff_callnative FldEff_UseSurf
	field_eff_end

gFieldEffectScript_GroundImpactDust:: @ 82DBB28
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_Dust
	field_eff_end

gFieldEffectScript_UseSecretPowerCave:: @ 82DBB32
	field_eff_callnative FldEff_UseSecretPowerCave
	field_eff_end

gFieldEffectScript_JumpTallGrass:: @ 82DBB38
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_JumpTallGrass
	field_eff_end

gFieldEffectScript_SandFootprints:: @ 82DBB42
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_SandFootprints
	field_eff_end

gFieldEffectScript_JumpBigSplash:: @ 82DBB4C
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_JumpBigSplash
	field_eff_end

gFieldEffectScript_Splash:: @ 82DBB56
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_Splash
	field_eff_end

gFieldEffectScript_JumpSmallSplash:: @ 82DBB60
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_JumpSmallSplash
	field_eff_end

gFieldEffectScript_LongGrass:: @ 82DBB6A
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_LongGrass
	field_eff_end

gFieldEffectScript_JumpLongGrass:: @ 82DBB74
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_JumpLongGrass
	field_eff_end

gFieldEffectScript_UnusedGrass:: @ 82DBB7E
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_UnusedGrass
	field_eff_end

gFieldEffectScript_UnusedGrass2:: @ 82DBB88
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_UnusedGrass2
	field_eff_end

gFieldEffectScript_UnusedSand:: @ 82DBB92
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_UnusedSand
	field_eff_end

gFieldEffectScript_WaterSurfacing:: @ 82DBB9C
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_WaterSurfacing
	field_eff_end

gFieldEffectScript_BerryTreeGrowthSparkle:: @ 82DBBA6
	field_eff_callnative FldEff_BerryTreeGrowthSparkle
	field_eff_end

gFieldEffectScript_DeepSandFootprints:: @ 82DBBAC
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_DeepSandFootprints
	field_eff_end

gFieldEffectScript_PokeCenterHeal:: @ 82DBBB6
	field_eff_loadfadedpal gSpritePalette_PokeballGlow
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_PokecenterHeal
	field_eff_end

gFieldEffectScript_UseSecretPowerTree:: @ 82DBBC5
	field_eff_callnative FldEff_UseSecretPowerTree
	field_eff_end

gFieldEffectScript_UseSecretPowerShrub:: @ 82DBBCB
	field_eff_callnative FldEff_UseSecretPowerShrub
	field_eff_end

gFieldEffectScript_TreeDisguise:: @ 82DBBD1
	field_eff_callnative ShowTreeDisguiseFieldEffect
	field_eff_end

gFieldEffectScript_MountainDisguise:: @ 82DBBD7
	field_eff_callnative ShowMountainDisguiseFieldEffect
	field_eff_end

gFieldEffectScript_NPCUseFly:: @ 82DBBDD
	field_eff_callnative FldEff_NPCFlyOut
	field_eff_end

gFieldEffectScript_UseFly:: @ 82DBBE3
	field_eff_callnative FldEff_UseFly
	field_eff_end

gFieldEffectScript_FlyIn:: @ 82DBBE9
	field_eff_callnative FldEff_FlyIn
	field_eff_end

gFieldEffectScript_QuestionMarkIcon:: @ 82DBBEF
	field_eff_callnative FldEff_QuestionMarkIcon
	field_eff_end

gFieldEffectScript_FeetInFlowingWater:: @ 82DBBF5
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_FeetInFlowingWater
	field_eff_end

gFieldEffectScript_BikeTireTracks:: @ 82DBBFF
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_BikeTireTracks
	field_eff_end

gFieldEffectScript_SandDisguisePlaceholder:: @ 82DBC09
	field_eff_callnative ShowSandDisguiseFieldEffect
	field_eff_end

gFieldEffectScript_UseRockSmash:: @ 82DBC0F
	field_eff_callnative FldEff_UseRockSmash
	field_eff_end

gFieldEffectScript_UseStrength:: @ 82DBC15
	field_eff_callnative FldEff_UseStrength
	field_eff_end

gFieldEffectScript_UseDig:: @ 82DBC1B
	field_eff_callnative FldEff_UseDig
	field_eff_end

gFieldEffectScript_SandPile:: @ 82DBC21
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_SandPile
	field_eff_end

gFieldEffectScript_ShortGrass:: @ 82DBC2B
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_ShortGrass
	field_eff_end

gFieldEffectScript_HotSpringsWater:: @ 82DBC35
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect1, FldEff_HotSpringsWater
	field_eff_end

gFieldEffectScript_UseWaterfall:: @ 82DBC3F
	field_eff_callnative FldEff_UseWaterfall
	field_eff_end

gFieldEffectScript_UseDive:: @ 82DBC45
	field_eff_callnative FldEff_UseDive
	field_eff_end

gFieldEffectScript_Pokeball:: @ 82DBC4B
	field_eff_loadpal gSpritePalette_Pokeball
	field_eff_callnative FldEff_Pokeball
	field_eff_end

gFieldEffectScript_HeartIcon:: @ 82DBC56
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_HeartIcon
	field_eff_end

gFieldEffectScript_Nop47:: @ 82DBC60
	field_eff_callnative FldEff_Nop47
	field_eff_end

gFieldEffectScript_Nop48:: @ 82DBC66
	field_eff_callnative FldEff_Nop48
	field_eff_end

gFieldEffectScript_AshPuff:: @ 82DBC6C
	field_eff_loadfadedpal_callnative gSpritePalette_Ash, FldEff_AshPuff
	field_eff_end

gFieldEffectScript_AshLaunch:: @ 82DBC76
	field_eff_loadfadedpal_callnative gSpritePalette_Ash, FldEff_AshLaunch
	field_eff_end

gFieldEffectScript_SweetScent:: @ 82DBC80
	field_eff_callnative FldEff_SweetScent
	field_eff_end

gFieldEffectScript_SandPillar:: @ 82DBC86
	field_eff_loadfadedpal_callnative gSpritePalette_SandPillar, FldEff_SandPillar
	field_eff_end

gFieldEffectScript_Bubbles:: @ 82DBC90
	field_eff_loadfadedpal_callnative gSpritePalette_GeneralFieldEffect0, FldEff_Bubbles
	field_eff_end

gFieldEffectScript_Sparkle:: @ 82DBC9A
	field_eff_loadfadedpal_callnative gSpritePalette_SmallSparkle, FldEff_Sparkle
	field_eff_end

gFieldEffectScript_ShowSecretPowerCave:: @ 82DBCA4
	field_eff_loadfadedpal_callnative gSpritePalette_SecretPower_Cave, FldEff_SecretPowerCave
	field_eff_end

gFieldEffectScript_ShowSecretPowerTree:: @ 82DBCAE
	field_eff_loadfadedpal_callnative gSpritePalette_SecretPower_Plant, FldEff_SecretPowerTree
	field_eff_end

gFieldEffectScript_ShowSecretPowerShrub:: @ 82DBCB8
	field_eff_loadfadedpal_callnative gSpritePalette_SecretPower_Plant, FldEff_SecretPowerShrub
	field_eff_end

gFieldEffectScript_ShowCutGrass:: @ 82DBCC2
	field_eff_loadfadedpal_callnative gSpritePalette_CutGrass, FldEff_CutGrass
	field_eff_end

gFieldEffectScript_FieldMoveShowMonInit:: @ 82DBCCC
	field_eff_callnative FldEff_FieldMoveShowMonInit
	field_eff_end

gFieldEffectScript_UsePuzzleEffect:: @ 82DBCD2
	field_eff_callnative FldEff_UsePuzzleEffect
	field_eff_end

gFieldEffectScript_SecretBaseBootPC:: @ 82DBCD8
	field_eff_callnative FldEff_SecretBasePCTurnOn
	field_eff_end

gFieldEffectScript_HallOfFameRecord:: @ 82DBCDE
	field_eff_loadfadedpal gSpritePalette_PokeballGlow
	field_eff_loadfadedpal_callnative gSpritePalette_HofMonitor, FldEff_HallOfFameRecord
	field_eff_end

gFieldEffectScript_UseTeleport:: @ 82DBCED
	field_eff_callnative FldEff_UseTeleport
	field_eff_end

gFieldEffectScript_RayquazaSpotlight:: @ 82DBCF3
	field_eff_callnative FldEff_RayquazaSpotlight
	field_eff_end

gFieldEffectScript_DestroyDeoxysRock:: @ 82DBCF9
	field_eff_callnative FldEff_DestroyDeoxysRock
	field_eff_end

gFieldEffectScript_MoveDeoxysRock:: @ 82DBCFF
	field_eff_callnative FldEff_MoveDeoxysRock
	field_eff_end
