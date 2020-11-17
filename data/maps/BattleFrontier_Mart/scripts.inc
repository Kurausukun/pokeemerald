.set LOCALID_OLD_WOMAN, 2

BattleFrontier_Mart_MapScripts:: @ 8267ACB
	.byte 0

BattleFrontier_Mart_EventScript_Clerk:: @ 8267ACC
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart BattleFrontier_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
BattleFrontier_Mart_Pokemart: @ 8267AE4
	.short ITEM_ULTRA_BALL
	.short ITEM_HYPER_POTION
	.short ITEM_MAX_POTION
	.short ITEM_FULL_RESTORE
	.short ITEM_FULL_HEAL
	.short ITEM_REVIVE
	.short ITEM_MAX_REPEL
	.short ITEM_PROTEIN
	.short ITEM_CALCIUM
	.short ITEM_IRON
	.short ITEM_ZINC
	.short ITEM_CARBOS
	.short ITEM_HP_UP
	.short ITEM_NONE
	release
	end

BattleFrontier_Mart_EventScript_OldMan:: @ 8267B02
	msgbox BattleFrontier_Mart_Text_ChaperonGrandson, MSGBOX_NPC
	end

BattleFrontier_Mart_EventScript_OldWoman:: @ 8267B0B
	lock
	applymovement LOCALID_OLD_WOMAN, Common_Movement_FaceDown
	waitmovement 0
	msgbox BattleFrontier_Mart_Text_ProteinMakeNiceGift, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Mart_EventScript_Boy:: @ 8267B20
	msgbox BattleFrontier_Mart_Text_FacilitiesDontAllowItems, MSGBOX_NPC
	end

BattleFrontier_Mart_Text_ChaperonGrandson: @ 8267B29
	.string "We came here to chaperon our\n"
	.string "grandson.\p"
	.string "But since we're here, we thought\n"
	.string "we should get some souvenirs.$"

BattleFrontier_Mart_Text_ProteinMakeNiceGift: @ 8267B8F
	.string "Dear, what do you think of this?\n"
	.string "Wouldn't this make a nice gift?\p"
	.string "It's…PRO…TE…IN?\n"
	.string "It sounds delicious, doesn't it?$"

BattleFrontier_Mart_Text_FacilitiesDontAllowItems: @ 8267C01
	.string "A lot of the BATTLE FRONTIER's\n"
	.string "facilities don't allow the use of items\l"
	.string "during battles.\p"
	.string "That rule makes things tougher than\n"
	.string "they already are!$"

