/ C library -- devstat
.ident	"@(#)libc-i386:sys/devstat.s	1.1"

/ devstat(char *pathp, int cmd, struct devstat *bufp)

	.globl	_cerror

_fwdef_(`devstat'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$DEVSTAT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
