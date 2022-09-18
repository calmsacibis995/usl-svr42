/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86at:util/in_outb.s	1.2"
	.file	"util/misc_io.s"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft 
/	Corporation and should be treated as Confidential.

/ The following routines read and write I/O adress space
/ outl(port address, val)
/ outw(port address, val)
/ outb(port address, val)
/ long  inw(port address)
/ ushort inw(port address)
/ unchar  inb(port address)

	.set	PORT, 8
	.set	VAL, 12

	.align	4
	/* XENIX Support */
	.type	outd,@function
	.globl	outd
outd:
	/* End XENIX Support */
	.type	outl,@function
	.globl	outl
outl:	pushl	%ebp
	movl	%esp, %ebp
	movw	PORT(%ebp), %dx
	movl	VAL(%ebp), %eax
	outl	(%dx)
	popl	%ebp
	ret
	.size	outd,.-outd
	.size	outl,.-outl

	.align	4
	.type	outw,@function
	.globl	outw
outw:	pushl	%ebp
	movl	%esp, %ebp
	movw	PORT(%ebp), %dx
	movw	VAL(%ebp), %ax
	data16
	outl	(%dx)
	popl	%ebp
	ret
	.size	outw,.-outw

	.align	4
	/* XENIX Support */
	.type	iooutb,@function
	.globl	iooutb
iooutb:
	/* End XENIX Support */
	.type	outb,@function
	.globl	outb
outb:	pushl	%ebp
	movl	%esp, %ebp
	movw	PORT(%ebp), %dx
	movb	VAL(%ebp), %al
	outb	(%dx)
	popl	%ebp
	ret
	.size	iooutb,.-iooutb
	.size	outb,.-outb

	.align	4
	/* XENIX Support */
	.type	ind,@function
	.globl	ind
ind:
	/* End XENIX Support */
	.type	inl,@function
	.globl	inl
inl:	pushl	%ebp
	movl	%esp, %ebp
	movw	PORT(%ebp), %dx
	inl	(%dx)
	popl	%ebp
	ret
	.size	ind,.-ind
	.size	inl,.-inl

	.align	4
	.type	inw,@function
	.globl	inw
inw:	pushl	%ebp
	movl	%esp, %ebp
	subl    %eax, %eax
	movw	PORT(%ebp), %dx
	data16
	inl	(%dx)
	popl	%ebp
	ret
	.size	inw,.-inw

	.align	4
	/* XENIX Support */
	.type	ioinb,@function
	.globl	ioinb
ioinb:
	/* End XENIX Support */
	.type	inb,@function
	.globl	inb
inb:	pushl	%ebp
	movl	%esp, %ebp
	subl    %eax, %eax
	movw	PORT(%ebp), %dx
	inb	(%dx)
	popl	%ebp
	ret
	.size	ioinb,.-ioinb
	.size	inb,.-inb

/
/ The following routines move strings to and from an I/O port.
/ loutw(port, addr, count);
/ linw(port, addr, count);
/* XENIX Support */
/ repinsw(port, addr, cnt) - input a stream of 16-bit words
/ repoutsw(port, addr, cnt) - output a stream of 16-bit words
/* End XENIX Support */
/
	.set	PORT, 8
	.set	ADDR, 12
	.set	COUNT, 16

	.align	4
	/* XENIX Support */
	.type	repoutsw,@function
	.globl	repoutsw
repoutsw:
	/* End XENIX Support */
	.type	loutw,@function
	.globl	loutw
loutw:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edx
	pushl	%esi
	pushl	%ecx
	movl	PORT(%ebp),%edx
	movl	ADDR(%ebp),%esi
	movl	COUNT(%ebp),%ecx
	rep
	data16
	outsl
	popl	%ecx
	popl	%esi
	popl	%edx
	popl	%ebp
	ret
	.size	repoutsw,.-repoutsw
	.size	loutw,.-loutw

	.align	4
	/* XENIX Support */
	.type	repinsw,@function
	.globl	repinsw
repinsw:
	/* End XENIX Support */
	.type	linw,@function
	.globl	linw
linw:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edx
	pushl	%edi
	pushl	%ecx
	movl	PORT(%ebp),%edx
	movl	ADDR(%ebp),%edi
	movl	COUNT(%ebp),%ecx
	rep
	data16
	insl
	popl	%ecx
	popl	%edi
	popl	%edx
	popl	%ebp
	ret
	.size	repinsw,.-repinsw
	.size	linw,.-linw

/* XENIX Support */
/	repinsb(port, addr, cnt) - input a stream of bytes
/	repinsd(port, addr, cnt) - input a stream of 32-bit words
/	repoutsb(port, addr, cnt) - output a stream of bytes
/	repoutsd(port, addr, cnt) - output a stream of 32-bit words

	.set	BPARGBAS, 8
	.set	io_port, BPARGBAS
	.set	io_addr, BPARGBAS+4
	.set	io_cnt,  BPARGBAS+8

/ repinsb(port, addr, count);
/ NOTE: count is a BYTE count

	.align	4
	.type	repinsb,@function
	.globl	repinsb
repinsb:
	push	%ebp
	mov	%esp,%ebp
	push	%edi

	mov	io_addr(%ebp),%edi
	mov	io_cnt(%ebp),%ecx
	mov	io_port(%ebp),%edx	/ read from right port

	rep
	insb				/ read them bytes

	pop	%edi
	pop	%ebp
	ret
	.size	repinsb,.-repinsb


/ repinsd(port, addr, count);
/ NOTE: count is a DWORD count

	.align	4
	.type	repinsd,@function
	.globl	repinsd
repinsd:
	push	%ebp
	mov	%esp,%ebp
	push	%edi

	mov	io_addr(%ebp),%edi
	mov	io_cnt(%ebp),%ecx
	mov	io_port(%ebp),%edx	/ read from right port

	rep
	insl				/ read them dwords

	pop	%edi
	pop	%ebp
	ret
	.size	repinsd,.-repinsd

/ repoutsb(port, addr, count);
/ NOTE: count is a byte count

	.align	4
	.type	repoutsb,@function
	.globl	repoutsb
repoutsb:
	push	%ebp
	mov	%esp,%ebp
	push	%esi

	mov	io_addr(%ebp),%esi
	mov	io_cnt(%ebp),%ecx
	mov	io_port(%ebp),%edx

	rep
	outsb

	pop	%esi
	pop	%ebp
	ret
	.size	repoutsb,.-repoutsb

/ repoutsd(port, addr, count);
/ NOTE: count is a DWORD count

	.align	4
	.type	repoutsd,@function
	.globl	repoutsd
repoutsd:
	push	%ebp
	mov	%esp,%ebp
	push	%esi

	mov	io_addr(%ebp),%esi
	mov	io_cnt(%ebp),%ecx
	mov	io_port(%ebp),%edx

	rep
	outsl

	pop	%esi
	pop	%ebp
	ret
	.size	repoutsd,.-repoutsd
/* End XENIX Support */
