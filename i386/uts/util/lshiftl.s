/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.



/ Shift a double long value.
/
/ The second argment, cnt, determines the direction of the shift:
/ a positive number indicates a left shift, and a negative number
/ a right shift.

	.ident	"@(#)uts-x86:util/lshiftl.s	1.4"
	.ident	"$Header: $"
        .file   "util/lshiftl.s"
	.type	lshiftl,@function
        .globl  lshiftl
        .text
        .align  4

	.set	arg,12
	.set	cnt,20
	.set	ans,0

lshiftl:	pushl	%eax
	movl	arg(%esp),%eax
	movl	arg+4(%esp),%edx
	movl	cnt(%esp),%ecx
	orl	%ecx,%ecx
	jz	.lshiftld
	jns	.lshiftlp

/ We are doing a negative (right) shift

	negl	%ecx

.lshiftln:
	sarl	$1,%edx
	rcrl	$1,%eax
	loop	.lshiftln
	jmp	.lshiftld

/ We are doing a positive (left) shift

.lshiftlp:
	shll	$1,%eax
	rcll	$1,%edx
	loop	.lshiftlp

/ We are done.

.lshiftld:
	movl	%eax,%ecx
	popl	%eax
	movl	%ecx,ans(%eax)
	movl	%edx,ans+4(%eax)

	ret	$4
	.size	lshiftl,.-lshiftl
