	.include "asm/macros.inc"
	thumb_func_start CopySoundMainRAM
CopySoundMainRAM::
	ldr r0, =.Ldata
	ldmia r0, {r0-r3}
	sub r2, r2, r0
	lsr r2, r2, #2
	bx r3

	.align 2
.Ldata:
	.word SoundMainRAM
	.word SoundMainRAM_Buffer
	.word SoundMainBTM
	.word CpuFastSet

	thumb_func_end CopySoundMainRAM
