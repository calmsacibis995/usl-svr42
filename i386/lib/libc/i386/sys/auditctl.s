/ C library -- auditctl
.ident	"@(#)libc-i386:sys/auditctl.s	1.1"

/ auditctl(cmd, actlp);

	.globl	_cerror

_fwdef_(`auditctl'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITCTL,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
