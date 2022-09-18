/ C library -- flvlfile
.ident	"@(#)libc-i386:sys/flvlfile.s	1.1"

	.globl	_cerror

_fwdef_(`flvlfile'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FLVLFILE,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
