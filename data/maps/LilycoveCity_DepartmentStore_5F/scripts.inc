.set LOCALID_WOMAN, 7

LilycoveCity_DepartmentStore_5F_MapScripts:: @ 821FF7A
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, LilycoveCity_DepartmentStore_5F_OnWarp
	.byte 0

@ During the abnormal weather from Kyogre/Groudon awakening the dept store rooftop is inaccessible
@ Likely done to avoid dealing with showing the weather. Technically the rooftop is indoors
LilycoveCity_DepartmentStore_5F_OnWarp: @ 821FF80
	map_script_2 VAR_SOOTOPOLIS_CITY_STATE, 1, LilycoveCity_DepartmentStore_5F_EventScript_BlockRoofStairs
	map_script_2 VAR_SOOTOPOLIS_CITY_STATE, 2, LilycoveCity_DepartmentStore_5F_EventScript_BlockRoofStairs
	map_script_2 VAR_SOOTOPOLIS_CITY_STATE, 3, LilycoveCity_DepartmentStore_5F_EventScript_BlockRoofStairs
	.short 0

LilycoveCity_DepartmentStore_5F_EventScript_BlockRoofStairs:: @ 821FF9A
	setobjectxy LOCALID_WOMAN, 16, 2
	turnobject LOCALID_WOMAN, DIR_NORTH
	end

LilycoveCity_DepartmentStore_5F_EventScript_ClerkFarLeft:: @ 821FFA6
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemartdecoration2 LilycoveCity_DepartmentStore_5F_Pokemart_Dolls
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
LilycoveCity_DepartmentStore_5F_Pokemart_Dolls: @ 821FFC0
	.short DECOR_PICHU_DOLL
	.short DECOR_PIKACHU_DOLL
	.short DECOR_MARILL_DOLL
	.short DECOR_JIGGLYPUFF_DOLL
	.short DECOR_DUSKULL_DOLL 
	.short DECOR_WYNAUT_DOLL
	.short DECOR_BALTOY_DOLL
	.short DECOR_KECLEON_DOLL
	.short DECOR_AZURILL_DOLL
	.short DECOR_SKITTY_DOLL
	.short DECOR_SWABLU_DOLL
	.short DECOR_GULPIN_DOLL
	.short DECOR_NONE
	release
	end

LilycoveCity_DepartmentStore_5F_EventScript_ClerkMidLeft:: @ 821FFDC
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemartdecoration2 LilycoveCity_DepartmentStore_5F_Pokemart_Cushions
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
LilycoveCity_DepartmentStore_5F_Pokemart_Cushions: @ 821FFF4
	.short DECOR_PIKA_CUSHION
	.short DECOR_ROUND_CUSHION
	.short DECOR_ZIGZAG_CUSHION
	.short DECOR_SPIN_CUSHION
	.short DECOR_DIAMOND_CUSHION
	.short DECOR_BALL_CUSHION
	.short DECOR_GRASS_CUSHION
	.short DECOR_FIRE_CUSHION
	.short DECOR_WATER_CUSHION
	.short DECOR_NONE
	release
	end

LilycoveCity_DepartmentStore_5F_EventScript_ClerkMidRight:: @ 822000A
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemartdecoration2 LilycoveCity_DepartmentStore_5F_Pokemart_Posters
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
LilycoveCity_DepartmentStore_5F_Pokemart_Posters: @ 8220024
	.short DECOR_BALL_POSTER
	.short DECOR_GREEN_POSTER
	.short DECOR_RED_POSTER
	.short DECOR_BLUE_POSTER
	.short DECOR_CUTE_POSTER
	.short DECOR_PIKA_POSTER
	.short DECOR_LONG_POSTER
	.short DECOR_SEA_POSTER
	.short DECOR_SKY_POSTER
	.short DECOR_NONE
	release
	end

LilycoveCity_DepartmentStore_5F_EventScript_ClerkFarRight:: @ 822003A
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemartdecoration2 LilycoveCity_DepartmentStore_5F_Pokemart_Mats
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
LilycoveCity_DepartmentStore_5F_Pokemart_Mats: @ 8220054
	.short DECOR_SURF_MAT
	.short DECOR_THUNDER_MAT
	.short DECOR_FIRE_BLAST_MAT
	.short DECOR_POWDER_SNOW_MAT
	.short DECOR_ATTRACT_MAT
	.short DECOR_FISSURE_MAT
	.short DECOR_SPIKES_MAT
	.short DECOR_GLITTER_MAT
	.short DECOR_JUMP_MAT
	.short DECOR_SPIN_MAT
	.short DECOR_NONE
	release
	end

LilycoveCity_DepartmentStore_5F_EventScript_PokefanF:: @ 822006C
	msgbox LilycoveCity_DepartmentStore_5F_Text_PlaceFullOfCuteDolls, MSGBOX_NPC
	end

LilycoveCity_DepartmentStore_5F_EventScript_Woman:: @ 8220075
	lockall
	applymovement LOCALID_WOMAN, Common_Movement_FacePlayer
	waitmovement 0
	compare VAR_SOOTOPOLIS_CITY_STATE, 0
	goto_if_eq LilycoveCity_DepartmentStore_5F_EventScript_WomanNormal
	compare VAR_SOOTOPOLIS_CITY_STATE, 4
	goto_if_ge LilycoveCity_DepartmentStore_5F_EventScript_WomanNormal
	goto LilycoveCity_DepartmentStore_5F_EventScript_WomanLegendaryWeather
	end

LilycoveCity_DepartmentStore_5F_EventScript_WomanNormal:: @ 822009C
	msgbox LilycoveCity_DepartmentStore_5F_Text_SellManyCuteMatsHere, MSGBOX_DEFAULT
	closemessage
	releaseall
	end

LilycoveCity_DepartmentStore_5F_EventScript_WomanLegendaryWeather:: @ 82200A7
	msgbox LilycoveCity_DepartmentStore_5F_Text_ClosedRooftopForWeather, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_WOMAN, Common_Movement_WalkInPlaceFastestUp
	waitmovement 0
	releaseall
	end

LilycoveCity_DepartmentStore_5F_EventScript_LittleGirl:: @ 82200BC
	msgbox LilycoveCity_DepartmentStore_5F_Text_GettingDollInsteadOfPokemon, MSGBOX_NPC
	end

LilycoveCity_DepartmentStore_5F_Text_PlaceFullOfCuteDolls: @ 82200C5
	.string "This place is full of cute DOLLS.\p"
	.string "I should buy some for me, instead of\n"
	.string "just for my children.$"

LilycoveCity_DepartmentStore_5F_Text_GettingDollInsteadOfPokemon: @ 8220122
	.string "I'm not big enough to raise POKéMON,\n"
	.string "so I'm getting a cute DOLL instead.$"

LilycoveCity_DepartmentStore_5F_Text_SellManyCuteMatsHere: @ 822016B
	.string "They sell many cute MATS here.\p"
	.string "I wonder which one I should get?\n"
	.string "Maybe I'll buy them all…$"

LilycoveCity_DepartmentStore_5F_Text_ClosedRooftopForWeather: @ 82201C4
	.string "I think they closed the rooftop\n"
	.string "because the weather is wild today.$"
