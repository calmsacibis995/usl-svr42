/ C library -- lvlproc
.ident	"@(#)libc-i386:sys/lvlproc.s	1.1"

/ lvlproc(int cmd, level_t *levelp)

	.globl	_cerror

_fwdef_(`lvlproc'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LVLPROC,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
