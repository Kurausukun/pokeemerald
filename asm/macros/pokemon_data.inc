	.macro pokedex_entry height, width, text_pointer, pokemon_scale, pokemon_offset, trainer_scale, trainer_offset
	.short \height @ in decimeters
	.short \width @ in hectograms
	.int \text_pointer
	.short 0 @ unused
	.short \pokemon_scale
	.short \pokemon_offset
	.short \trainer_scale
	.short \trainer_offset
	.short 0 @ padding
	.endm

	.macro base_stats hp, attack, defense, speed, sp_attack, sp_defense
	.byte \hp
	.byte \attack
	.byte \defense
	.byte \speed
	.byte \sp_attack
	.byte \sp_defense
	.endm

	.macro ev_yield hp, attack, defense, speed, sp_attack, sp_defense
	.short (\sp_defense << 10) | (\sp_attack << 8) | (\speed << 6) | (\defense << 4) | (\attack << 2) | \hp
	.endm

	.macro level_up_move level, move
	.short (\level << 9) | \move
	.endm

	.macro evo_entry method, parameter, target_species
	.short \method
	.short \parameter
	.short \target_species
	.short 0 @ padding
	.endm

	.macro empty_evo_entries count
	.fill 8 * \count, 1, 0
	.endm

	.macro egg_moves_begin species
	.short 20000 + \species
	.endm

@ If the min level equals the max level, only one level argument is needed.
	.macro wild_mon species, min_level, max_level
	.byte \min_level

	.ifb \max_level
	.byte \min_level
	.else
	.byte \max_level
	.endif

	.short \species
	.endm
