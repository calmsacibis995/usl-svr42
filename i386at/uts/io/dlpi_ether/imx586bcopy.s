/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.file	"imx586bcopy.s"
/ imx586bcopy(p_from, p_to, count)
/ long *p_from, *p_to, count; /* count is in bytes */
/ this routine is used to copy data on and off of imx586 board
/ the imx586 board MUST be accessed on 16 bit boundries only
/ see imx586 hardware ref man
/ this routine copies in 4 byte increments and rounds down 

	.ident	"@(#)uts-x86at:io/dlpi_ether/imx586bcopy.s	1.2"
	.ident	"$Header: $"
	.text

	.globl	imx586bcopy

imx586bcopy:

	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	push	%edi
	movl	0x08(%ebp),%esi
	movl	0x0c(%ebp),%edi
	movl	0x10(%ebp),%ecx
	shrl	$2,%ecx
	orl	%ecx,%ecx
	jz	copy_done
	cld
	rep 
	smovl
copy_done:
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
