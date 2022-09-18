/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


/
/	 Copyrighted as an unpublished work.
/	 (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
/	 All rights reserved.
/	
/	 RESTRICTED RIGHTS
/	
/	 These programs are supplied under a license.  They may be used,
/	 disclosed, and/or copied only as permitted under such license
/	 agreement.  Any copy must contain the above copyright notice and
/	 this restricted rights notice.  Use, copying, and/or disclosure
/	 of the programs is strictly prohibited unless otherwise provided
/	 in the license agreement.

	.file	"ix_util.s"

	.ident	"@(#)uts-x86at:boot/at386/ix_util.s	1.1"
	.ident	"$Header: $"

	.text

/ Do a BIOS int call.  Called by doint(&param)
/
/		where: param -> struct intc {
/					short ax,bx,cx,dx,bp,es;
/					}
	.globl	doint
doint:
	push	%esi
	mov	8(%esp), %esi
	pusha
	movw	(%esi),%ax	/ pickup the int code to do
	movb	%al, intcode+1
	call	goreal		/ get real
	push	%esi
	sti
	addr16
	mov	2(%esi),%eax	/ setup registers for the call
	addr16
	mov	4(%esi),%ebx
	addr16
	mov	6(%esi),%ecx
	addr16
	mov	8(%esi),%edx
	addr16
	mov	10(%esi),%ebp
	addr16
	movw	12(%esi),%es
intcode:
	int	$0x10		/ do BIOS call
	cli
	pop	%esi
	pushf			/ save carry for later
	pushl	%eax		/ and stash the returned registers
	pushl	%ebx		/ that are going to get trashed
	pushl	%es		/ by goprot call
	data16
	call	goprot		/ protect mode
	popw	%ax
	movw	%ax, 12(%esi)	/ pop the saved registers(shorts)
	popw	%ax		/ transfering them to the int call
	movw	%ax, 4(%esi)	/ param block
	popw	%ax
	movw	%ax, 2(%esi)
	movw	%cx, 6(%esi)
	movw	%dx, 8(%esi)
	movw	%bp, 10(%esi)
	xorl	%eax, %eax	/ initalize return to zero
	popfw			/ get carry
	jnc	dixt
	inc	%eax		/ return != 0 for carry set
dixt:
	movl	%eax, 0x1c(%esp)	/ set return value
	popa
	pop	%esi
	ret
int_0:
	int	$0x0

