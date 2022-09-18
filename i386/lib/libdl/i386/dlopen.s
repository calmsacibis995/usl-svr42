/ident	"@(#)libdl:i386/dlopen.s	1.2"
/ dlopen calls _dlopen in ld.so

	.type	dlopen,@function
	.globl	dlopen
	.globl	_dlopen

dlopen:
	jmp	_dlopen@PLT
