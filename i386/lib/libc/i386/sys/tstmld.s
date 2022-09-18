/ C library -- tstmld
.ident	"@(#)libc-i386:sys/tstmld.s	1.1"

/ tstmld(char *path)

	.globl	_cerror

_fwdef_(`tstmld'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$TSTMLD,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
