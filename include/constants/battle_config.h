#ifndef GUARD_CONSTANTS_BATTLE_CONFIG_H
#define GUARD_CONSTANTS_BATTLE_CONFIG_H

#define SPECIES_ZYGARDE_10 10005 // 10 %
#define SPECIES_ZYGARDE_COMPLETE 10006 // 100 %

#define GEN_3 0
#define GEN_4 1
#define GEN_5 2
#define GEN_6 3
#define GEN_7 4
#define GEN_8 5

// Calculation settings
#define B_CRIT_CHANCE               GEN_6 // Chances of a critical hit landing. See CalcCritChanceStage.
#define B_CRIT_MULTIPLIER           GEN_6 // In Gen6+, critical hits multiply damage by 1.5 instead of 2.
#define B_EXP_CATCH                 GEN_6 // In Gen6+, Pokémon get experience from catching.
#define B_TRAINER_EXP_MULTIPLIER    GEN_6 // In Gen7+, trainer battles no longer give a 1.5 multiplier to exp gain.
#define B_BURN_DAMAGE               GEN_7 // In Gen7+, burn damage is 1/16th of max HP instead of 1/8th.
#define B_PARALYSIS_SPEED           GEN_7 // In Gen7+, speed is decreased by 50% instead of 75%.
#define B_TERRAIN_TYPE_BOOST        GEN_8 // In Gen8+, speed is boosted by 30% instead of 50%.
#define B_BINDING_DAMAGE            GEN_6 // In Gen6+, binding damage is 1/8 of max HP instead of 1/16. (With Binding Band, 1/6 and 1/8 respectively.)
#define B_CONFUSION_SELF_DMG_CHANCE GEN_7 // In Gen7+, confusion has a 33.3% of self-damage, instead of 50%.
#define B_MULTI_HIT_CHANCE          GEN_6 // In Gen5+, multi-hit moves have different %. See Cmd_setmultihitcounter for values.
#define B_RECOIL_IF_MISS_DMG        GEN_6 // In Gen5+, Jump Kick and Hi Jump Kick will always do half of the user's max HP when missing.
#define B_PSYWAVE_DMG               GEN_6 // Psywave's damage formula. See Cmd_psywavedamageeffect.

// Move settings
#define B_FELL_STINGER_STAT_RAISE   GEN_7 // In Gen7+, it raises Atk by 3 stages instead of 2 if it causes the target to faint.
#define B_SOUND_SUBSTITUTE          GEN_6 // In Gen6+, sound moves bypass Substitute.
#define B_TOXIC_NEVER_MISS          GEN_6 // In Gen6+, if Toxic is used by a Poison type, it will never miss.
#define B_PAYBACK_SWITCH_BOOST      GEN_6 // In Gen5+, if the opponent switches out, Payback's damage will no longer be doubled.
#define B_KINGS_SHIELD_LOWER_ATK    GEN_7 // In Gen7+, it lowers Atk by 1 stage instead of 2 of oponents that hit it.
#define B_BINDING_TURNS             GEN_6 // In Gen5+, binding moves last for 4-5 turns instead of 2-5 turns. (With Grip Claw, 7 and 5 turns respectively.)
#define B_UPROAR_TURNS              GEN_6 // In Gen5+, Uproar lasts for 3 turns instead of 2-5 turns.
#define B_DISABLE_TURNS             GEN_6 // Disable's turns. See Cmd_disablelastusedattack.
#define B_INCINERATE_GEMS           GEN_6 // In Gen6+, Incinerate can destroy Gems.
#define B_MINIMIZE_DMG_ACC          GEN_6 // In Gen6+, moves that causes double damage to minimized Pokémon will also skip accuracy checks.

// Ability settings
#define B_ABILITY_POP_UP            GEN_6 // In Gen5+, the Pokémon abilities are displayed in a pop-up, when they activate in battle.
#define B_ABILITY_WEATHER           GEN_6 // In Gen5+, weather caused by abilities lasts the same amount of turns as induced from a move. Before, they lasted till the battle's end or weather change by a move.
#define B_GALE_WINGS                GEN_7 // In Gen7+ requires full HP to trigger.
#define B_STANCE_CHANGE_FAIL        GEN_7 // In Gen7+, Aegislash's form change does not happen, if the Pokémon cannot use a move, because of confusion, paralysis, etc. In gen6, the form change occurs despite not being able to move.
#define B_GHOSTS_ESCAPE             GEN_6 // In Gen6+, ghosts can escape even when blocked by abilities such as Shadow Tag.
#define B_MOODY_ACC_EVASION         GEN_8 // In Gen8+, Moody CANNOT raise Accuray and Evasion any more.
#define B_FLASH_FIRE_FROZEN         GEN_6 // In Gen5+, Flash Fire can trigger even when frozen, when it couldn't before.
#define B_SYNCHRONIZE_NATURE        GEN_8 // In Gen8+, if the Pokémon with Synchronize is leading the party, it's 100% guaranteed that wild Pokémon will have the same ability, as opposed to 50% previously.

// Item settings
#define B_HP_BERRIES                GEN_6 // In Gen4+, berries which restore hp activate immediately after hp drops to half. In gen3, the effect occurs at the end of the turn.
#define B_BERRIES_INSTANT           GEN_6 // In Gen4+, most berries activate on battle start/switch-in if applicable. In gen3, they only activate either at the move end or turn end.

// Other
#define B_FLAG_INVERSE_BATTLE       0     // If this flag is set, the battle's type effectiveness are inversed. For example, fire is super effective against water. 0 disables the feature.
#define B_FAST_INTRO                TRUE  // If set to TRUE, battle intro texts print at the same time as animation of a Pokémon, as opposing to waiting for the animation to end.
#define B_SHOW_TARGETS              TRUE  // If set to TRUE, all available targets, for moves hitting 2 or 3 Pokémon, will be shown before selecting a move.
#define B_SLEEP_TURNS               GEN_6 // In Gen5+, sleep lasts for 1-3 turns instead of 2-5 turns.
#define B_PARALYZE_ELECTRIC         GEN_6 // In Gen6+, Electric type Pokémon can't be paralyzed.
#define B_POWDER_GRASS              GEN_6 // In Gen6+, Grass type Pokémon are immune to powder and spore moves.

// Animation Settings
#define NEW_SWORD_PARTICLE              FALSE    // update swords dance particle
#define NEW_LEECH_SEED_PARTICLE         FALSE    //update leech seed's animation particle
#define NEW_HORN_ATTACK_PARTICLE        FALSE    //update horn attack's horn
#define NEW_LEAF_PARTICLE               FALSE    // update leaf particle
#define NEW_EMBER_PARTICLES             FALSE    //updates ember fire particle
#define NEW_MEAN_LOOK_PARTICLE          FALSE    //update mean look eye
#define NEW_TEETH_PARTICLE              FALSE    //update bite/crunch teeth particle
#define NEW_HANDS_FEET_PARTICLE         FALSE    //update chop/kick/punch particles
#define NEW_SPIKES_PARTICLE             FALSE    //update spikes particle
#define NEW_FLY_BUBBLE_PARTICLE         FALSE    //update fly 'bubble' particle
#define NEW_CURSE_NAIL_PARTICLE         FALSE    //updates curse nail
#define NEW_BATON_PASS_BALL_PARTICLE    FALSE    //update baton pass pokeball sprite
#define NEW_MORNING_SUN_STAR_PARTICLE   FALSE    //updates morning sun star particles
#define NEW_IMPACT_PALETTE              FALSE    //updates the basic 'hit' particle
#define NEW_SURF_PARTICLE_PALETTE       FALSE    //updates the surf wave palette

#endif // GUARD_CONSTANTS_BATTLE_CONFIG_H
