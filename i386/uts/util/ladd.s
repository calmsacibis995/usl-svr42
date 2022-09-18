/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


/ Double long add routine.
	.ident	"@(#)uts-x86:util/ladd.s	1.3"
	.ident	"$Header: $"
        .file   "util/ladd.s"
        .text

	.type	ladd,@function
        .globl  ladd
        .align  4
	.set	lop,8
	.set	rop,16
	.set	ans,0

ladd:
	movl	lop(%esp),%ecx
	addl	rop(%esp),%ecx
	movl	lop+4(%esp),%edx
	adcl	rop+4(%esp),%edx
	movl	%ecx,ans(%eax)
	movl	%edx,ans+4(%eax)

	ret	$4
	.size   ladd,.-ladd
