/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft 
/	Corporation and should be treated as Confidential.

	.ident	"@(#)uts-x86at:svc/uprt.s	1.5"
	.ident	"$Header: $"

include(../util/symbols.m4)

/
/	Declare externs:
/
	.globl	gdt
	.globl  gdtend
	.globl	idt
ifdef(`VPIX',`
	.globl	idt2
')
	.globl	kpt0
	.globl	kptn
	.globl	scall_dscr
	.globl	sigret_dscr
	.globl	df_stack
	.globl	stext

	.set	PROTBIT, 0x0001
	.set	PAGEBIT, 0x80000000
	.set    DFSTKSIZ,0x0FFE
	.set	IDTLIM, [8\*256-1]
	.set	BM_BASE, 0
	.set	BM_EXTENT, 4
	.set	SIZEOF_BM, 12
	.set	KPD_LOC, [KPTBL_LOC+0x1000]

/
/	*** NOTICE *** NOTICE *** NOTICE *** NOTICE *** NOTICE ***
/
/		The instructions in pstart are reversed 16 <--> 32
/		bits.  This is because we are running pstart in
/		REAL MODE.  By using long instructions, we generate
/		opcodes that are 16 bit instructions when run
/		in REAL MODE.

/	More nice information:
/		This code now only supports the BKI boot-kernel interface.
/		This passes the magic number 0xff1234ff in %edi.
/		All other info is passed in the bootinfo structure.

	.text
	.globl  uprt_start
	.globl	pstart
	.type	pstart,@function
uprt_start:
pstart:
	data16
	cmpl	$BKI_MAGIC, %edi
	data16
	je	.BKI_ok

	/ Bad magic number from bootstrap.
	/ Print a message, then halt.
	/ Unfortunately, this will only work on an AT386.

	data16
	call	R_Print
	.string	"\r\nBootstrap too old.\r\n"
.halt:
	sti
	hlt
	jmp	.halt


	.align	8	/ This is for ease of looking at memory.
.Rgdtdscr:
	.value  0       / Filled in later with ((gdtend - gdt) & ~7) - 1.
	.long	gdt

	.align	8
.Ridtdscr:
	.value	IDTLIM
	.long	idt

/	EGA font pointers (these start as real mode pointers)
/	The pointers point to the 8x8, 8x14, 9x14, 8x16 and 
/	the 9x16 fonts, respectively

	.globl	egafontptr
	.type	egafontptr,@object
egafontptr:
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.size	egafontptr,.-egafontptr
	
.BKI_ok:
/	Here we have to set up the kernel symbol page table
/	according to the memused information passed by the bootstrap.
/	After were done, R_Set_Addr will be able to convert virtual
/	addresses to physical addresses using this page table.
	data16
	xorl	%eax, %eax		/ Load 0 into segment registers
	movw	%ax, %ds		/   so we get absolute addresses
	movw	%ax, %es
	movw	%ax, %fs
	cld

	data16
	movl	$KPTBL_LOC, %edi	/ First zero out the page table & dir
	data16
	movl	$2048, %ecx
	addr16
	data16
	rep; sstol

	data16
	movl	$BOOTINFO_LOC, %ebx
	data16
	addr16
	movl	memusedcnt(%ebx), %edx	/ Get count of memused segments
	data16
	movl	%edx, %esi
	data16
	addl	$memused, %ebx		/ Get pointer to first segment

	data16
	movl	$KPTBL_LOC, %edi	/ "Reserved" segment maps at KVSBASE

.kptbl_loop:
	data16
	addr16
	movl	BM_EXTENT(%ebx), %ecx	/ Compute # pages for this segment
	data16
	shrl	$12, %ecx

	data16
	addr16
	movl	BM_BASE(%ebx), %eax	/ Compute base pte for this segment
	data16
	andl	$0xfffff000, %eax
	incl	%eax			/ Set present bit

.kptseg_loop:
	addr16
	data16
	sstol				/ Store the next page table entry
	data16
	addl	$0x1000, %eax		/ Advance to next physical page
	loop	.kptseg_loop

	data16
	cmpl	%edx, %esi		/ If moving on to 2nd segment,
	jne	.kptbl_next		/ Reset %esi for start of text

	data16
	movl	$stext, %edi		/ Compute addr of page table
	data16
	shrl	$12-2, %edi		/  entry for start of kernel text
	data16
	andl	$0xffc, %edi
	data16
	addl	$KPTBL_LOC, %edi

.kptbl_next:
	data16
	addl	$SIZEOF_BM, %ebx	/ Advance to next segment
	decl	%edx
	data16
	jnz	.kptbl_loop

/	At this point, we are running on the bootloaders stack.
/	We will now find our stack and switch to it.
	data16
	movl	$df_stack, %eax
	data16
	call	R_Set_Addr

	movw	%ds, %ax
	movw	%ax, %ss
	data16
	addl    $DFSTKSIZ, %ebx
	movw	%bx, %sp

	/ Now, find the GDT so that we can rearrange it.
	data16
	movl	$gdt, %eax

	data16
	movl    $gdtend, %ecx
	subw	%ax, %cx
	data16
	decl	%ecx

	data16
	movl	$.Rgdtdscr, %eax
	data16
	call	R_Set_Addr
	addr16
	movl	%ecx, (%ebx)

	data16
	movl	$gdt, %eax
	data16
	call	munge_table

	/ Find the IDT so that we can rearrange it.
	data16
	movl	$idt, %eax

	data16
	movl	$IDTLIM, %ecx
	data16
	call	munge_table
ifdef(`VPIX',`
	/ Find the IDT so that we can rearrange it.
	data16
	movl	$idt2, %eax

	data16
	movl	$IDTLIM, %ecx
	data16
	call	munge_table
')

	/ A couple of other interesting descriptors.  (scall_dscr)
	data16
	movl    $scall_dscr, %eax
	data16
	movl    $1, %ecx
	data16
	call	munge_table

	/ A couple of other interesting descriptors.  (sigret_dscr)
	data16
	movl    $sigret_dscr, %eax
	data16
	movl    $1, %ecx
	data16
	call	munge_table

	/ Now, we need to fix up the first, 3gig, and last entries in the
	/ page directory.
	data16
	movl	$kpt0, %eax		/ First, Page table 0
	data16
	call	R_Virt_to_Phys
	incl	%eax			/ Set the present bit
	addr16
	movw	%ax, %fs:KPD_LOC
	addr16
	movw	%ax, %fs:[KPD_LOC+3072]	/ KVBASE page directory entry

	data16				/ Also, kernel address page table
	movl	$KPTBL_LOC+1, %eax	/   (with present bit set)
	addr16
	movw	%ax, %fs:[KPD_LOC+3328]	/ KVSBASE page directory entry

	data16
	movl	$kptn, %eax		/ Now, the last Page table
	data16
	call	R_Virt_to_Phys
	incl	%eax			/ Set the present bit
	addr16
	movw	%ax, %fs:[KPD_LOC+4092]

	/ Load IDTR and GDTR
	data16
	movl    $.Rgdtdscr, %eax
	data16
	call	R_Set_Addr
	addr16
	data16
	lgdt	(%ebx)

/	Code to find font locations from the bios
/	and to put them in egafonptr[] where the kd driver can find them.
	movw	$0x1130, %ax	/ set up bios call
	.value	0
	movw	$0x0300, %bx	/ get pointer to 8x8 font
	.value	0
	int	$0x10

	data16
	movl	$egafontptr, %eax
	data16
	call	R_Set_Addr
	addr16
	movl	%ebp, (%ebx)
	addr16
	movw	%es, 2(%ebx)

	movw	$0x1130, %ax	/ set up bios call
	.value	0
	movw	$0x0200, %bx	/ get pointer to 8x14 font
	.value	0
	int	$0x10

	data16
	movl	$egafontptr+4, %eax
	data16
	call	R_Set_Addr
	addr16
	movl	%ebp, (%ebx)
	addr16
	movw	%es, 2(%ebx)

	movw	$0x1130, %ax	/ set up bios call
	.value	0
	movw	$0x0500, %bx	/ get pointer to 9x14 font
	.value	0
	int	$0x10

	data16
	movl	$egafontptr+8, %eax
	data16
	call	R_Set_Addr
	addr16
	movl	%ebp, (%ebx)
	addr16
	movw	%es, 2(%ebx)

	movw	$0x1130, %ax	/ set up bios call
	.value	0
	movw	$0x0600, %bx	/ get pointer to 8x16 font
	.value	0
	int	$0x10

	data16
	movl	$egafontptr+0xc, %eax
	data16
	call	R_Set_Addr
	addr16
	movl	%ebp, (%ebx)
	addr16
	movw	%es, 2(%ebx)

	movw	$0x1130, %ax	/ set up bios call
	.value	0
	movw	$0x0700, %bx	/ get pointer to 9x16 font
	.value	0
	int	$0x10

	data16
	movl	$egafontptr+0x10, %eax
	data16
	call	R_Set_Addr
	addr16
	movl	%ebp, (%ebx)
	addr16
	movw	%es, 2(%ebx)

/	*** NOTICE *** NOTICE *** NOTICE *** NOTICE *** NOTICE ***
/
/		Do not try to single step past this point!!!!
/		use a go till command!!!!
	data16
	movl    $.Ridtdscr, %eax
	data16
	call	R_Set_Addr
	addr16
	data16
	lidt	(%ebx)

	movl	%cr0, %eax
	data16
	orl     $PROTBIT, %eax		/ Kick us into protected mode
	movl	%eax, %cr0

	/ First thing in protected mode must be a jump to flush prefetch queue
	jmp	.qflush
.qflush:

			/ Note that this point we are still
			/ in 16 bit addressing mode.
	data16
	movl	$KPD_LOC, %eax		/ Special phys addr reserved for
	movl	%eax, %cr3		/   page directory

	movl	%cr0, %eax
	data16
	orl	$PAGEBIT, %eax		/ Enable paging (virtual mode)
	movl	%eax, %cr0

	data16
	movl    $JTSSSEL, %eax

	ltr	%ax

/	This is a 16 bit long jump.
	.byte	0xEA
	.value	0
	.value	KTSSSEL

	.size	pstart,.-pstart
	

/ *********************************************************************
/
/	munge_table:
/		This procedure will munge a descriptor table to
/		change it from initialized format to runtime format.
/
/		Assumes:
/			%eax -- contains the base address of table.
/			%ecx -- contains size of table.
/
/ *********************************************************************
	.local	munge_table
	.type	munge_table,@function
munge_table:
	pushl	%ds

	data16
	andl    $0xFFFF, %ecx
	addw	%ax, %cx
	movw	%ax, %si

.moretable:
	cmpw	%si, %cx
	jl	.donetable		/ Have we done every descriptor??

	movw	%si, %ax
	data16
	call	R_Set_Addr

	addr16
	movb	7(%ebx), %al	/ Find the byte containing the type field
	testb	$0x10, %al	/ See if this descriptor is a segment
	jne	.notagate
	testb	$0x04, %al	/ See if this descriptor is a gate
	je	.notagate
				/ Rearrange a gate descriptor.
	addr16
	movl	6(%ebx), %eax	/ Type (etc.) lifted out
	addr16
	movl	4(%ebx), %edx	/ Selector lifted out.
	addr16
	movl	%eax, 4(%ebx)	/ Type (etc.) put back
	addr16
	movl	2(%ebx), %eax	/ Grab Offset 16..31
	addr16
	movl	%edx, 2(%ebx)	/ Put back Selector
	addr16
	movl	%eax, 6(%ebx)	/ Offset 16..31 now in right place
	jmp	.descdone

.notagate:			/ Rearrange a non gate descriptor.
	addr16
	movl	4(%ebx), %edx	/ Limit 0..15 lifted out
	addr16
	movb	%al, 5(%ebx)	/ type (etc.) put back
	addr16
	movl	2(%ebx), %eax	/ Grab Base 16..31
	addr16
	movb	%al, 4(%ebx)	/ put back Base 16..23
	addr16
	movb	%ah, 7(%ebx)	/ put back Base 24..32
	addr16
	movl	(%ebx), %eax	/ Get Base 0..15
	addr16
	movl	%eax, 2(%ebx)	/ Base 0..15 now in right place
	addr16
	movl	%edx, (%ebx)	/ Limit 0..15 in its proper place

.descdone:
	data16
	addl	$8, %esi	/ Go for the next descriptor
	jmp	.moretable

.donetable:
	popl	%ds
	data16
	ret
	.size	munge_table,.-munge_table


/ *********************************************************************
/
/	R_Virt_to_Phys:
/		This procedure takes a 32 bit virtual address and
/		converts it to a linear physical address by looking
/		it up in the kernel page table.
/
/		Input:
/			%eax -- virtual address.
/
/		Output:
/			%eax -- physical address.
/
/ *********************************************************************
	.local	R_Virt_to_Phys
	.type	R_Virt_to_Phys,@function
R_Virt_to_Phys:
	data16
	pushl	%ebx
	data16
	movl	%eax, %ebx		/ Save virtual address in %ebx
	data16
	subl	$KVSBASE, %eax		/ If address is below KVSBASE,
	jb	.vtop_done		/   assume its physical

	data16
	shrl	$12-2, %eax		/ Convert to page table offset
	data16
	andl	$0xffc, %eax

	data16
	addr16
	movl	%fs:KPTBL_LOC(%eax), %eax  / Get physical page address
	data16
	andl	$0xfffff000, %eax

	data16
	andl	$0xfff, %ebx		/ Get virtual page offset

	data16
	addl	%eax, %ebx		/ Compute final virtual address

.vtop_done:
	data16
	movl	%ebx, %eax
	data16
	popl	%ebx
	data16
	ret
	.size	R_Virt_to_Phys,.-R_Virt_to_Phys


/ *********************************************************************
/
/	R_Set_Addr:
/		This procedure takes a 32 bit address and sets up ds:bx
/		from it.  In the process, it will cut it down into
/		the first megabyte.
/
/		Input:
/			%eax -- 32-bit virtual address.
/
/		Output:
/			%ds:%ebx -- real-mode address.
/			[ %eax not preserved ]
/
/ *********************************************************************
	.local	R_Set_Addr
	.type	R_Set_Addr,@function
R_Set_Addr:
	data16
	call	R_Virt_to_Phys	/ Convert to a physical address.
	data16
	movl	%eax, %ebx	/ Remember the addr for the offset portion.
	data16
	shrl	$4, %eax	/ Turn the address into a real mode selector.
	movw	%ax, %ds

	data16
	andl    $0xF, %ebx      / Now the offset portion

	data16
	ret
	.size	R_Set_Addr,.-R_Set_Addr


/ *********************************************************************
/
/	R_Print
/
/	x86at-specific print-string function (calls BIOS print chr)
/
/ *********************************************************************
	.local	R_Print
	.type	R_Print,@function
R_Print:
	data16
	popl	%esi		/ get pointer to message

	movb	$1, %bl		/ foreground color
.ploop:	addr16
	movb	%cs:(%esi), %al	/ get chr
	data16
	incl	%esi
	testb	%al, %al	/ test for end of string
	jz	.pend
	movb	$14, %ah	/ setup call to bios
	int	$0x10		/ print chr
	jmp	.ploop		/ repeat for next chr
.pend:
	data16
	pushl	%esi
	data16
	ret
	.size	R_Print,.-R_Print
