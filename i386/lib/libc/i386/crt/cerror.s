	.file	"cerror.s"

	.ident	"@(#)libc-i386:crt/cerror.s	1.4"

/ C return sequence which sets errno, returns -1.

	.set	ERESTART,91
	.set	EINTR,4

	.globl	_cerror
	.globl	errno

_fgdef_(_cerror):
	_prologue_
	/
	/ Restartable calls intercept ERESTART in the system call stub.
	/ We'll see ERESTART here only for those system calls that are
	/ NOT restartable; for these, the caller should get EINTR instead.
	/
	cmpl	$ERESTART,%eax
	jne	.L1
	movl	$EINTR,%eax
.L1:
_m4_ifdef_(`DSHLIB',
`	movl	_daref_(errno),%ecx
	movl	%eax,(%ecx)
',
`	movl	%eax,errno
')
	movl	$-1,%eax
	_epilogue_
	ret
