/ C library -- lvlfile
.ident	"@(#)libc-i386:sys/lvlfile.s	1.1"

/ lvlfile(char *pathp, int cmd, level_t *levelp)

	.globl	_cerror

_fwdef_(`lvlfile'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LVLFILE,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
