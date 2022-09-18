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

	.file	"priboot.s"

	.ident	"@(#)uts-x86at:boot/at386/priboot.s	1.2"
	.ident  "$Header: $"

/ 	UNIX primary boot program

include(priboot.m4)

/ 	Here we are running where we are originally loaded:
/	cs = 0x0, ip = 0x7c00.

	.text	
	.set	BOOTSECTS,PROG_SECT+1	/ # of secondary boot sectors 	
	.set	RETRIES, 3		/ max I/O error retries.
	.set	SECBOOT_MEM_LOC, SECBOOT_ADDR   / secondary boot memory location
	.set	PRIBOOT_STACK, PRIBOOT_ADDR-2	/ primary boot stack base
ifdef(`WINI',`
/ from fdisk.h
	.set	IPART,0x600+B_BOOTSZ
	.set	BOOTIND,0
	.set	RELSECT,8
	.set	IPART_SZ,16
	.set	BOOTDRV,BOOTDRV_HD
',`
	.set	BOOTDRV,BOOTDRV_FP
')

	.globl	_start
ZERO:
_start:
	cli
	jmp past_spt
	.align	4
drv_mark:
	.long	0		/ reserved space

past_spt:
	movw	%cs, %ax

	movw	%ds, %bx	/ Save ds for access to fdisk entry
	movw	%ax, %ds

	movw	%ax, %es
	movw	%ax, %ss	/ sanitize the stack segment register
	data16
	movl	$PRIBOOT_STACK, %esp
	sti			/ allow interrupts

/	save ds:si entry address
	data16
	shll	$4, %ebx
	data16
	addl	%esi, %ebx
	addr16
	data16
	movl	%ebx, ZERO

/	data16
/	mov	$ldmsg, %esi
/	data16
/	call	strput

/ 	Get the boot drive parameters

	movb	$8, %ah			/ Call the BIOS to get drive parms
	movb	$BOOTDRV, %dl		/ ...for drive 0
	int	$0x13
	jnc	gotparm			/ jump if no error
ifdef(`WINI',`
	data16
	mov	$hdperr, %esi
	jmp	fatal
',`
	addr16
	data16
	mov	$FDB_ADDR, %eax		/ get floppy parameter table
	mov	%eax, %edi
	shrw	$16, %ax		/ 32 bit shift
	movw	%ax, %es
	movb	$1, %dh			/ max head, 0 based
	xor	%eax, %eax
	addr16
	movb	%es:0x4(%edi), %al	/ sectors per track
	xor	%ecx, %ecx
	addr16
	movb	%es:0xB(%edi), %cl	/ max cylinders
	jmp	setparm
')

gotparm:
	xor	%eax, %eax
	movb	%cl, %al
	andb	$0x3f, %al		/ extract spt value
	shrb	$6, %cl			/ align cyl hi bits
	xchgb	%cl, %ch		/ make into a short
setparm:
	addr16
	mov	%eax, spt		/ and save it
/	addr16
/	mov	%ecx, mxcyl
	incb	%dh			/ convert track to 1 base
	addr16
	movb	%dh, tpc		/ save tracks per cyl(heads)
	mulb	%dh			/ make sectors per cyl
	addr16
	mov	%eax, spc		/ and save it

ifdef(`WINI',`
/	BIOS entry address pointed by ds:si specifies the fdisk partition
/	that was used to locate this primary boot program.
/	(1) check for valid ds:si entry address. If valid, use that as
/	    active partition address. Otherwise, reset to zero.
/	(2) record the active partition address; zero indicates no active
/	    partition.

	data16
	xorl	%edx, %edx			/ set invalid flag for ds:si 
	addr16
	data16
	movl	ZERO, %esi			/ address for ds:si entry
	data16
	xorl	%ebx, %ebx			/ default active partition addr
	addr16
	data16					/ a full 32 bit address for
	movl	$IPART, %eax			/ partition table address
	movb	$B_FD_NUMPART, %cl		/ number of fdisk partitions
ostry:
	addr16
	cmpb	$B_ACTIVE, BOOTIND(%eax)	/ check active partition
	jne	not_actpart			/ branch if not active partition
	data16
	movl	%eax, %ebx			/ save active partition
not_actpart:
	data16
	cmpl	%eax, %esi			/ check for valid ds:si entry 
	jne	nomatch				/ branch if not match
	incb	%dl				/ set valid flag
nomatch:	
	addr16
	data16
	addl	$IPART_SZ, %eax			/ next partition
	loop	ostry

	addr16
	data16
	movl	%ebx, act_part		/ save active partition location
	testb	%dl, %dl		/ if invalid ds:si entry address
	jz	bad_entry		/ then skip
	data16
	movl	%esi, %ebx		/ get saved ds:si entry location
	jmp	osfound			

bad_entry:
	addr16
	data16
	movl	%edx, ZERO		/ set invalid ds:si entry address
	data16
	testl	%ebx, %ebx		/ if valid active partition exists
	jnz	osfound			/ then found active partition	

        data16				
        movl    $nopart, %esi           / error: no active partition found
        data16
        jmp     fatal

osfound:
	addr16
	data16
	movl	RELSECT(%ebx), %eax	/ save relative sector number
	data16
	addr16
	movl	%eax, unix_start
	data16
	inc	%eax		/ plus one to get over primary
	data16
	addr16
	movl	%eax, readsect
')

/ 	call the BIOS to read the remainder of the bootstrap from disk
/	all the data is set in the diskblock

ifdef(`WINI',`

	data16
	call	RDdisk			/ do the i/o
',`
	data16
	mov	$BOOTSECTS, %ecx
doiof:
	push	%ecx
	data16
	call	RDdisk			/ read a floppy sector
	addr16
	mov	bps, %eax		/ get bytes per sector (16 bits)
	data16
	addr16
	add	%eax, destadrs		/ advance read address (32 bits)
	data16
	addr16
	inc	readsect		/ bump the sector to read
	pop	%ecx
	loop	doiof
')

/	pass data to the secondary boot program through stack
setstack:
	data16
	xorl	%eax, %eax		/ clear eax
	addr16
	movl	act_part, %eax		/ ptr to active partition
	data16
	pushl	%eax

	addr16
	movl	ZERO, %eax		/ ds:si index for active partition
	data16
	pushl	%eax

	addr16
	mov	bps, %eax		/ bytes per sector
	data16
	pushl	%eax

	addr16
	movl	spt, %eax		/ sectors per track
	data16
	pushl	%eax

	addr16
	movl	spc, %eax		/ sector per cylinder
	data16
	pushl	%eax

	data16
	movl	$SECBOOT_MEM_LOC, %eax	/ load address for sec boot program
	data16
	pushl	%eax

/	make an indirect jump to the secondary boot program
/	%eax got the address of the secondary boot program
	addr16				/ add bps as word to ax
	addl	bps, %eax		/ skip 1 sector for boot control blk
	data16
	addr16
	call	*%eax

/	no return here
/	----------------------------------------------------
/
/	Stop everything, an error occured.
/

	.globl	fatal
fatal:
	data16
	call	strput			/ print error message
					/ fall through to...
	.globl	halt

halt:	sti				/ allow int's
	hlt				/ stop.
	jmp	halt			/ STOP.

/	----------------------------------------------------
/ 	strput:		put null-terminated string at si to console
/
	.globl	strput
strput:
	movb	$1, %bl		/ select a normal video attribute
stploop:
	cld			/ make sure we are going the right dir
	lodsb			/ pickup a message byte
	orb	%al,%al
	jz	pexit		/ zero is end of the string

	movb	$14, %ah	/ Write TTY function
	int	$0x10
	jmp	stploop
okread:
pexit:
	data16
	ret			


/	----------------------------------------------------
/
/	RDdisk makes bios calls to read from the boot drive based
/	on the information in the disk parameters block.
/	Here we are in real mode.
/	the segment:offset destination.  secno is 0 for the
/	first sector on the disk.  count is the number of
/	sectors to read.
/
	.globl	RDdisk
RDdisk:	
	movb	$RETRIES, %cl		/ set retry counter
retry:
	push	%ecx			/ save the retry counter
	addr16
	mov	readsect,%eax		/ get the relative sector to read
	addr16
	mov	readsect+2,%edx		/ get the relative sector to read
	addr16
	div	spc			/ long divide to get cyl
	movw	%ax, %cx		/ cyl to cx
	xchgb	%cl, %ch		/ low number to high
	shlb	$6, %cl			/ align cyl upper 2 bits
	movw	%dx, %ax		/ remainder from divide to ax
	cltd				/ make a long in ax,dx
	addr16
	div	spt			/ divide to get track(head)
	movb	%al, %dh		/ to dh for int call
	orb	%dl, %cl		/ starting sector to cl
	incb	%cl			/ convert to 1 based
	movb	$BOOTDRV, %dl		/ get boot drive number
	addr16
	data16
	mov	destadrs, %eax		/ pickup the destination phys addr
	xor	%ebx,%ebx		/ prep
	movb	%al, %bl		/ offset part
	andb	$0x0f, %bl		/ only a nibbles worth
	shrl	$4, %eax			/ paragraph part
	movw	%ax, %es		/ to seg register es
	addr16
	movb	numsect, %al		/ sectors to read
	movb	$2, %ah			/ read sectors function

	int	$0x13			/ BIOS disk support
	pop	%ecx

	jnb	okread			/ retry if error
	cmpb	0x11, %ah		/ ECC corrected
	je	okread			/ jump yes

	push	%ecx
	movb	$0, %ah			/ reset controller
	int	$0x13
	pop	%ecx
	decb	%cl
	jnz	retry

	.globl	ioerr
ioerr:
	data16
	mov	$readerr, %esi
	data16	
	jmp	fatal

readerr:	.string		"Boot read error\r\n"
nopart:		.string		"No active partition on HD\r\n"
hdperr:		.string		"No HD params\r\n"
/ldmsg:		.string		"Loader 1.0\r\n\n"


	.align	4

	.globl	act_part
	.globl	spt
	.globl	spc
	.globl	readsect
	.globl	destadrs
	.globl	numsect
	.globl	bps
	.globl	tpc
/	.globl	mxcyl
/
/	disk I/O stuff, used to pass info for disk parameters and
/	I/O requests.
/
destadrs:	.long	SECBOOT_MEM_LOC	/ address(physical) for next read
readsect:	.long	1		/ starting sector to read (0 based)
act_part:	.long	0

ifdef(`WINI',`
	.globl	unix_start
unix_start:	.long	0		/ starting sector for unix partition
numsect:	.value	BOOTSECTS
',`
numsect:	.value	1		/ read 1 sector at a time for floppy
')

/	The following values are floppy defaults in case the BIOS does
/	not properly support the "read disk parameters" function.

bps:		.value	512		/ bytes per sector
spt:		.value	15		/ disk sectors per track
spc:		.value	15\*2		/ disk sectors per cylinder
tpc:		.value	2		/ tracks(heads) per cylinder
/mxcyl:		.value	79		/ max cyl number

/ With changeover to 486 compatible assembler, placement of bytes in exact
/ locations acts differently than previous version. Previously the WINI version
/ of the boot code used ZERO + 502, now ZERO + 510 is used. If booting from
/ the hard disk yields "missing operating system" message the dskmark (55AA) 
/ did not land at the last two bytes of the first sector as required.
ifdef(`WINI',`
	. = ZERO + 510
',`
	. = ZERO + 506
')
dskmark:
	.byte	0x55
	.byte	0xAA
