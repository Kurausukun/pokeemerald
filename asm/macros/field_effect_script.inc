	.macro field_eff_loadtiles address
	.byte 0
	.int \address
	.endm

	.macro field_eff_loadfadedpal address
	.byte 1
	.int \address
	.endm

	.macro field_eff_loadpal address
	.byte 2
	.int \address
	.endm

	.macro field_eff_callnative address
	.byte 3
	.int \address
	.endm

	.macro field_eff_end
	.byte 4
	.endm

	.macro field_eff_loadgfx_callnative tiles_address, palette_address, function_address
	.byte 5
	.int \tiles_address
	.int \palette_address
	.int \function_address
	.endm

	.macro field_eff_loadtiles_callnative tiles_address, function_address
	.byte 6
	.int \tiles_address
	.int \function_address
	.endm

	.macro field_eff_loadfadedpal_callnative palette_address, function_address
	.byte 7
	.int \palette_address
	.int \function_address
	.endm
