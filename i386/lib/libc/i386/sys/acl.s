/ C library -- acl
.ident	"@(#)libc-i386:sys/acl.s	1.1"

/ acl(file, cmd, nentries, buffer)

	.globl	_cerror

_fwdef_(`acl'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$ACL,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
