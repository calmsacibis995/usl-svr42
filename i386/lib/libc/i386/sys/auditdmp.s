/ C library -- auditdmp
.ident	"@(#)libc-i386:sys/auditdmp.s	1.1"

/ auditdmp(arecp);

	.globl  _cerror

_fwdef_(`auditdmp'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITDMP,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
