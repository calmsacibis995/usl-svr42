/ident	"@(#)libdl:i386/dlerror.s	1.2"
/ dlerror calls _dlerror in ld.so

	.type	dlerror,@function
	.globl	dlerror
	.globl	_dlerror

dlerror:
	jmp	_dlerror@PLT
