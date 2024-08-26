	.include "asm/macros/function.inc"
	
	.text
	
	thumb_func_start GetBiosChecksum
GetBiosChecksum:
	svc #13
	bx lr
	thumb_func_end GetBiosChecksum