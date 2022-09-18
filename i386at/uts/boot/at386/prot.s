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

	.file	"prot.s"

	.ident	"@(#)uts-x86at:boot/at386/prot.s	1.2"
	.ident  "$Header: $"

include(prot.m4)

/	_start(secboot_mem_loc, spc, spt, bps, entry_ds_si, act_part)
	.globl	_start
_start:
        cli
        data16
        call    goprot

	movl	[STK_SBML\*4](%esp), %eax
	movl	%eax, secboot_mem_loc
	movl	[STK_SPC\*4](%esp), %eax
	movw	%ax, spc
	movl	[STK_SPT\*4](%esp), %eax
	movw	%ax, spt
	movl	[STK_BPS\*4](%esp), %eax
	movw	%ax, bps
	movl	[STK_EDS\*4](%esp), %eax
	movl	%eax, entry_ds_si
	movl	[STK_AP\*4](%esp), %eax
	movl	%eax, act_part
	
/	zero out the bss
	movl	$edata, %edi		/ set start address at end of data seg
	movl	$end, %eax		/ get long word count
	subl	%edi, %eax
	shrl	$2, %eax
	incl	%eax			/ add 1 word count for truncation
	movl	%eax, %ecx
	xorl	%eax, %eax		/ set target address to zero
	rep
	stosl

	movl	$bstack, %eax
	leal	[SECBOOT_STACKSIZ-2](%eax), %eax
	movl	%eax, %esp

	call	main

/	no return here
	sti
	hlt

/	----------------------------------------------------
/ Enter protected mode.
/
/ We must set up the GDTR
/
/ When we enter this routine, 	ss = ds = cs = "codebase", 
/	when we leave,  	ss = D_GDT(0x08),
/				cs = C_GDT(0x10).
/				ds = es = B_GDT(0x20).
/
/ Trashes %ax, %bx and sets segment registers as above. 
/ CAUTION - If other than ax and bx get used, check all callers
/           to see that register(s) are saved around the call.

	.globl	goprot
goprot:

	data16
	popl	%ebx			/ get return %eip, for later use

/	load the GDTR

	addr16
	data16
	lgdt	GDTptr

/	data16				/ set the page directory pointer
/	movl	$0x2000, %eax
/	addr16
/	movl	%eax, %cr3

	mov	%cr0, %eax

	addr16
	data16
	orl	cr0mask, %eax		/ set protect mode and possibly
					/ page mode based on cr0mask.
	mov	%eax, %cr0 

	jmp	qflush			/ flush the prefetch queue

/ 	Set up the segment registers, so we can continue like before;
/ 	if everything works properly, this shouldn't change anything.
/ 	Note that we're still in 16 bit operand and address mode, here, 
/ 	and we will continue to be until the new %cs is established. 

qflush:
	data16
	mov	$B_GDT, %eax		/ big flat data descriptor
	movw	%ax, %ds
	movw	%ax, %es
/	data16
/	mov	$D_GDT, %eax		/ data descriptor
	movw	%ax, %ss		/ don't need to set %sp

/ 	Now, set up %cs by fiddling with the return stack and doing an lret

	data16
	pushl	$C_GDT			/ push %cs

	data16
	pushl	%ebx			/ push %eip

	data16
	lret

/	----------------------------------------------------
/ 	Re-enter real mode.
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

	.globl	goreal
goreal:

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

farjump:

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

/	Data definitions
	.align	4
	.comm	bstack, SECBOOT_STACKSIZ

	.align	4
	.globl	secboot_mem_loc
	.globl	act_part
	.globl	entry_ds_si
	.globl	cr0mask
	.globl	spc
	.globl	spt
	.globl	bps
secboot_mem_loc:
	.long	0
act_part:
	.long	0
entry_ds_si:
	.long	0
cr0mask:
	.long	PROTMASK
spc:
	.value	0
spt:
	.value	0
bps:
	.value	0

/	----------------------------------------------------
/ 	The GDTs for protected mode operation
/
/	All 32 bit GDTs can reference the entire 4GB address space.

	.globl	flatdesc

GDTstart:
nulldesc:			/ offset = 0x0

	.value	0x0	
	.value	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	

flatdesc:			/ offset = 0x08    B_GDT

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23; set for 0K
	.byte	0x92		/ flags; A=0, Type=001, DPL=00, P=1
				/        Present expand down
	.byte	0xCF		/ flags; Limit (16..19)=1111, AVL=0, G=1, B=1
	.byte	0x0		/ segment base 24..32

codedesc:			/ offset = 0x10    C_GDT

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23; set for 0k
	.byte	0x9E		/ flags; A=0, Type=111, DPL=00, P=1
	.byte	0x4F		/ flags; Limit (16..19)=1111, AVL=0, G=1, D=1
	.byte	0x0		/ segment base 24..32

code16desc:			/ offset = 0x18    C16GDT

	.value	0xFFFF		/ segment limit 0..15
	.value	0x000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23; set for 0k
	.byte	0x9E		/ flags; A=0, Type=111, DPL=00, P=1
	.byte	0x0F		/ flags; Limit (16..19)=1111, AVL=0, G=0, D=0
	.byte	0x0		/ segment base 24..32

datadesc:			/ offset = 0x20    D_GDT

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23; set for 0K
	.byte	0x92		/ flags; A=0, Type=001, DPL=00, P=1
				/        Present expand down
	.byte	0x4F		/ flags; Limit (16..19)=1111, AVL=0, G=1, B=1
	.byte	0x0		/ segment base 24..32


/	In-memory GDT pointer for the lgdt call

	.globl GDTptr
	.globl gdtlimit
	.globl gdtbase

GDTptr:	
gdtlimit:
	.value	0x28		/ size of GDT table
gdtbase:
	.long	GDTstart


