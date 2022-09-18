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

	.file	"util.s"

	.ident	"@(#)uts-x86at:boot/at386/util.s	1.1"
	.ident  "$Header: $"

/	protected mode putchar; character on stack.

	.globl	putchar
putchar:
	push	%ebp			/ save stack
	mov	%esp,%ebp
	pushl	%ebx

	call	goreal
	sti

	xorl	%ebx,%ebx		/ use page zero (%bh)
	movb	$1, %bl
	addr16
	movb	8(%ebp), %al		
	movb	$14, %ah		/ teletype putchar
	int	$0x10			/ issue request

	cli
	data16
	call	goprot

	popl	%ebx
	pop	%ebp			/ restore frame pointer

	ret

/	----------------------------------------------------
/ Return TRUE if a character is waiting to be read.
/
	.globl	ischar

ischar:
	push	%ebp			/ C entry
	mov	%esp,%ebp

	push	%ebx
	push	%edi
	push	%esi

	call	goreal
	sti

	data16
	mov	$0, %edx		/ clear %ecx for result

	movb	$1, %ah			/ setup for bios test for a char
	int	$0x16			/ sets the zero flag if char is waiting

	jz	nochar			/ no char waiting

	data16
	mov	$1, %edx		/ char waiting: return TRUE

nochar:
	cli
	data16
	call	goprot	

	pop	%esi			/ C exit
	pop	%edi
	pop	%ebx

	pop	%ebp

	mov	%edx, %eax		/ setup return; goprot trashes %eax

	ret


/	--------------------------------------------
/ 	Call BIOS wait routine to wait for 1 second; programming the interval
/	timer directly does not seem to be reliable.
/ 	- decreased resolution to cut down on overhead of mode switching
/

	.globl	wait1s
wait1s:
	push	%ebp			/ C entry
	mov	%esp,%ebp
	push	%edi
	push	%esi
	push	%ebx

	call	goreal
	sti

	mov	$0x0f, %ecx
	mov	$0x4240, %edx
	movb	$0x86, %ah		/ setup for bios wait
	int	$0x15			/ BIOS utility function

	mov	$0, %eax

	cli
	data16
	call	goprot

	pop	%ebx
	pop	%esi			/ C exit
	pop	%edi
	pop	%ebp

	ret


/	--------------------------------------------
/	memcpy(dest, src, cnt): works in exactly the same fashion as the 
/	libc memcpy; addresses are physaddr's.
	.globl	memcpy
memcpy:
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	cld
	movl	16(%esp), %edi	/ %edi = dest address
	movl	20(%esp), %esi	/ %esi = source address
	movl	24(%esp), %ecx	/ %ecx = length of string
	movl	%edi, %eax	/ return value from the call

	movl	%ecx, %ebx	/ %ebx = number of bytes to move
	shrl	$2, %ecx	/ %ecx = number of words to move
	rep ; smovl		/ move the words

	movl	%ebx, %ecx	/ %ecx = number of bytes to move
	andl	$0x3, %ecx	/ %ecx = number of bytes left to move
	rep ; smovb		/ move the bytes

	popl	%ebx
	popl	%esi
	popl	%edi
	ret
