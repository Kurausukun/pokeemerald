	.include "asm/macros/asm.inc"
	.include "asm/macros/function.inc"
	.include "asm/macros/movement.inc"
	.include "asm/macros/window.inc"
	.include "asm/macros/pokemon_data.inc"
	.include "asm/macros/ec.inc"
	.include "asm/macros/map.inc"
	.include "asm/macros/field_effect_script.inc"
	.include "asm/macros/trainer_hill.inc"
	.include "asm/macros/battle_tent.inc"
	.include "asm/macros/battle_frontier/apprentice.inc"
	.include "asm/macros/battle_frontier/battle_arena.inc"
	.include "asm/macros/battle_frontier/battle_dome.inc"
	.include "asm/macros/battle_frontier/battle_factory.inc"
	.include "asm/macros/battle_frontier/battle_palace.inc"
	.include "asm/macros/battle_frontier/battle_pike.inc"
	.include "asm/macros/battle_frontier/battle_pyramid.inc"
	.include "asm/macros/battle_frontier/battle_tower.inc"
	.include "asm/macros/battle_frontier/frontier_util.inc"

	.macro region_map_entry x, y, width, height, name
	.byte \x
	.byte \y
	.byte \width
	.byte \height
	.int gMapName_\name
	.endm

	.macro obj_tiles address, uncompressed_size, tag = 0
	.int \address
	.short \uncompressed_size
	.short \tag
	.endm

	.macro null_obj_tiles
	obj_tiles 0, 0, 0
	.endm

	.macro obj_pal address, tag
	.int \address
	.short \tag
	.short 0@ padding
	.endm

	.macro null_obj_pal
	obj_pal 0, 0
	.endm

	.macro paired_pals tag, address
	.short \tag
	.short 0 @ padding
	.int \address
	.endm

@ For object animation frames.
	.macro obj_frame_tiles address, uncompressed_size
	.int \address
	.short \uncompressed_size
	.short 0 @ padding
	.endm

	.macro spr_template tile_tag, pal_tag, oam, anims, images, affine_anims, callback
	.short \tile_tag
	.short \pal_tag
	.int \oam
	.int \anims
	.int \images
	.int \affine_anims
	.int \callback
	.endm

@ Berry trees have a table defining the palette slot used for each of their 5
@ stages. However, the first 2 stages always use the same slots regardless of
@ the type of tree and the slots of the last 3 stages always equal each other.
	.macro berry_tree_palette_slot_table slot
	.byte 3, 4, \slot, \slot, \slot
	.endm

	.macro subsprite x, y, priority, tile_num_offset, size
	.byte \x
	.byte \y
	.short ((\priority) << 14) | ((\tile_num_offset) << 4) | SPRITE_SIZE_\size
	.endm

	.macro obj_image_anim_frame pic_id, duration, flags = 0
	.short \pic_id
	.byte (\flags) | (\duration)
	.byte 0 @ padding
	.endm

	.macro obj_image_anim_loop count
	.short 0xfffd
	.byte \count
	.byte 0 @ padding
	.endm

	.macro obj_image_anim_jump target_index
	.short 0xfffe
	.byte \target_index
	.byte 0 @ padding
	.endm

	.macro obj_image_anim_end
	.short 0xffff
	.short 0 @ padding
	.endm

	.macro obj_rot_scal_anim_frame delta_x_scale, delta_y_scale, delta_angle, duration
	.short \delta_x_scale
	.short \delta_y_scale
	.byte \delta_angle
	.byte \duration
	.short 0 @ padding
	.endm

	.macro obj_rot_scal_anim_loop count
	.short 0x7ffd
	.short \count
	.int 0 @ padding
	.endm

	.macro obj_rot_scal_anim_jump target_index
	.short 0x7ffe
	.short \target_index
	.int 0 @ padding
	.endm

	.macro obj_rot_scal_anim_end unknown=0
	.short 0x7fff
	.short \unknown
	.fill 4 @ padding
	.endm

	.macro door_anim_frame unknown, offset
	.byte \unknown
	.byte 0 @ padding
	.short \offset
	.endm

	.macro door_anim_gfx metatile_num, unknown, unknown2, tile_addr, palette_addr
	.short \metatile_num
	.byte \unknown
	.byte \unknown2
	.int \tile_addr
	.int \palette_addr
	.endm
