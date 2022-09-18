/ C library -- lvlipc
.ident	"@(#)libc-i386:sys/lvlipc.s	1.1"

/ lvlipc(int type, int id, int cmd, level_t *levelp)

	.globl	_cerror

_fwdef_(`lvlipc'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LVLIPC,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
