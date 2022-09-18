/ident	"@(#)libdl:i386/dlclose.s	1.2"
/ dlclose calls _dlclose in ld.so

	.type	dlclose,@function
	.globl	dlclose
	.globl	_dlclose

dlclose:
	jmp	_dlclose@PLT
