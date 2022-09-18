/ C library -- auditevt
.ident	"@(#)libc-i386:sys/auditevt.s	1.1"

/ auditevt(cmd, aevtp)

	.globl	_cerror

_fwdef_(`auditevt'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITEVT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
