/ident	"@(#)libdl:i386/dlsym.s	1.2"
/ dlsym calls _dlsym in ld.so

	.type	dlsym,@function
	.globl	dlsym
	.globl	_dlsym

dlsym:
	jmp	_dlsym@PLT
