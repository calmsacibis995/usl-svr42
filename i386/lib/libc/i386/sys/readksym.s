.ident	"@(#)libc-i386:sys/readksym.s	1.1"

	.file	"readksym.s"
	
	.text

	.globl	_cerror

_fwdef_(`readksym'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$READKSYM,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret

