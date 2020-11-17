	.section script_data, "aw", %progbits

	.align 2
gMysteryEventScriptCmdTable:: @ 82DED2C
	.int MEScrCmd_nop                 @ 0x00
	.int MEScrCmd_checkcompat         @ 0x01
	.int MEScrCmd_end                 @ 0x02
	.int MEScrCmd_setmsg              @ 0x03
	.int MEScrCmd_setstatus           @ 0x04
	.int MEScrCmd_runscript           @ 0x05
	.int MEScrCmd_initramscript       @ 0x06
	.int MEScrCmd_setenigmaberry      @ 0x07
	.int MEScrCmd_giveribbon          @ 0x08
	.int MEScrCmd_givenationaldex     @ 0x09
	.int MEScrCmd_addrareword         @ 0x0a
	.int MEScrCmd_setrecordmixinggift @ 0x0b
	.int MEScrCmd_givepokemon         @ 0x0c
	.int MEScrCmd_addtrainer          @ 0x0d
	.int MEScrCmd_enableresetrtc      @ 0x0e
	.int MEScrCmd_checksum            @ 0x0f
	.int MEScrCmd_crc                 @ 0x10
gMysteryEventScriptCmdTableEnd::
