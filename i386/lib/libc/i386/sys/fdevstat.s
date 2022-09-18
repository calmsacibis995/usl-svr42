/ C library -- fdevstat
.ident	"@(#)libc-i386:sys/fdevstat.s	1.1"

/ fdevstat(int fildes, int cmd, struct devstat *bufp)

	.globl	_cerror

_fwdef_(`fdevstat'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FDEVSTAT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
