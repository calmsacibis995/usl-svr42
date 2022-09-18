/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.file	"vcopy.s"

	.ident	"@(#)uts-x86at:mem/vcopy.s	1.2"
	.ident	"$Header: $"

	.text

/ This routine moves bytes in memory.  It is used in the display driver.
/ vcopy(from, to, count, direction)
/	direction = 0 means from and to are the high addresses (move up)
/	direction = 1 means from and to are the low addresses (move down)
	.set	FROM, 8
	.set	TO, 12
	.set	COUNT, 16
	.set	DIR, 20

	.align	4
	.type	vcopy,@function
	.globl	vcopy
vcopy:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	movl	FROM(%ebp), %esi
	movl	TO(%ebp), %edi
	movl	COUNT(%ebp), %ecx
	movl	DIR(%ebp), %eax
	orl	%eax, %eax
	jnz	.doit		/ direction flag is zero by default
	std			/ move up; start at high end and decrement
.doit:
	rep
	movsw
	cld			/ clear direction flag
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
	.size	vcopy,.-vcopy
