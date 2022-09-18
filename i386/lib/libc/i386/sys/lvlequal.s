/ C library -- lvlequal
.ident	"@(#)libc-i386:sys/lvlequal.s	1.1"

/ lvlequal(level_t *level1p, level_t *level2p)

	.globl	_cerror

_fwdef_(`lvlequal'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LVLEQUAL,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
