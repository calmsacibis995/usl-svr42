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

	.file	"kswitch.s"

	.ident	"@(#)uts-x86at:boot/at386/sip/kswitch.s	1.4"
	.ident	"$Header: $:"

include(kswitch.m4)

/
/	Explicitly turn off the EM (emulate coprocessor) bit so that
/	BIOS' that support emulation won't mess up our floating point
/	detection code.  (With EM set, the first fp instruction causes
/	a trap, but we aren't prepared for it in the kernel.)  The EM
/	bit is off when the processor resets, and the kernel assumes
/	it will be that way when it gets control.  Initprogs that
/	do their own fp detection must be wary of this.  We didn't
/	put the code up front because we're afraid that a BIOS might
/	want to take advantage of this itself...

/	Entering the kernel through a task switch.

	.globl	kswitch
kswitch:

	lgdt	KGDTptr
	lidt	KIDTptr

	movl	$KPD_LOC, %eax
	movl	%eax, %cr3

	movl	%cr0, %eax
	orl	$[CR0_PG|PROTMASK], %eax/ set page and protected mode
	andl	$-1![CR0_EM], %eax	/ turn off emulation bit
	movl	%eax, %cr0 

	jmp	kflush			/ flush the prefetch queue
kflush:
	nop
	nop

	movl	$JTSSSEL, %eax
	ltr	%ax
	ljmp	$KTSSSEL, $0x0		/ jmp 150:0

/	data definitions

	.globl KGDTptr
	.globl kgdtlimit
	.globl kgdtbase

	.align	8
KGDTptr:
kgdtlimit:
	.value	0
kgdtbase:
	.long	0

	.globl KIDTptr
	.globl kidtlimit
	.globl kidtbase

	.align	8
KIDTptr:
kidtlimit:
	.value	0
kidtbase:
	.long	0

/	prepare a long jump into kernel in the real mode
/	kjump(address)
	.globl	kjump
kjump:
	mov	4(%esp), %eax		/ get future %cs
	mov	%eax, %ecx		/ get future %ip
	andl	$0x000f, %ecx
	shr	$4, %eax		
	andl	$0xffff, %eax

	movw	%ax,2(%esp)		/ set calling program segment
	movw	%cx,(%esp)		/ and offset
	push	%eax

	call	enter_real

	data16
	pop	%eax

	movw	%ax, %ds		/ set up %ds and %es for...
	movw	%ax, %es		/ program to be called

/ 	put magic number (0xff1234ff) into %edi

	data16
	mov	$B_BKI_MAGIC, %edi

	cli				/ turn off interrupts (for kernel)
	lret

/	----------------------------------------------------
/ 	Enter real mode.
/ 
/ 	We assume that we are executing code in a segment that
/ 	has a limit of 64k. Thus, the CS register limit should
/ 	be set up appropriately for real mode already. We also
/ 	assume that paging has *not* been turned on.
/ 	Set up %ss, %ds, %es, %fs, and %gs with a selector that
/ 	points to a descriptor containing the following values
/
/	Limit = 64k
/	Byte Granular 	( G = 0 )
/	Expand up	( E = 0 )
/	Writable	( W = 1 )
/	Present		( P = 1 )
/	Base = any value

	.globl	enter_real
enter_real:

/ 	To start off, transfer control to a 16 bit code segment

	ljmp	$C16GDT, $set16cs
set16cs:			/ 16 bit addresses and operands 

	data16
	movl	$D_GDT, %eax	/ need to have all segment regs sane ...
	movw	%ax, %es	/ ... before we can enter real mode
	movw	%ax, %ds

	data16
	mov	%cr0, %eax

	data16
	and 	$NOPROTMASK, %eax	/ clear the protection bit

	data16
	mov	%eax, %cr0

/	We do a long jump here, to reestablish %cs is real mode.
/	It appears that this has to be a ljmp as opposed to
/       a lret probably due to the way Intel fixed errata #25
/       on the A2 step. This leads to self modifying code.

	ljmp	$0x0, $restorecs

/ 	Now we've returned to real mode, so everything is as it 
/	should be. Set up the segment registers and so on.
/	The stack pointer can stay where it was, since we have fiddled
/	the segments to be compatible.

restorecs:

	data16
	mov	%cr3, %eax
	data16
	mov	%eax, %cr3
	movw	%cs, %ax
	movw	%ax, %ss
	movw	%ax, %ds
	movw	%ax, %es

	data16
	ret			/ return to whence we came; it was a 32 bit call


