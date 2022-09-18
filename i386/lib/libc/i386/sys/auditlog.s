/ C library -- auditlog
.ident	"@(#)libc-i386:sys/auditlog.s	1.1"

/ auditlog(cmd, alogp);

	.globl	_cerror

_fwdef_(`auditlog'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITLOG,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
