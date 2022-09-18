/ C library -- aclipc
.ident	"@(#)libc-i386:sys/aclipc.s	1.1"

/ aclipc(cmd, priv_vec, count)

	.globl	_cerror

_fwdef_(`aclipc'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$ACLIPC,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
