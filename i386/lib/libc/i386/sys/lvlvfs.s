/ C library -- lvlvfs
.ident	"@(#)libc-i386:sys/lvlvfs.s	1.1"

/ lvlvfs(const char *path, int cmd, level_t *hilevelp)

	.globl	_cerror

_fwdef_(`lvlvfs'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LVLVFS,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
