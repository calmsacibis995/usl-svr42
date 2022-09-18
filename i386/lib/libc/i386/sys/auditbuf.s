/ C library -- auditbuf
.ident	"@(#)libc-i386:sys/auditbuf.s	1.1"

/ auditbuf(cmd, abufp);

	.globl	_cerror

_fwdef_(`auditbuf'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITBUF,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
