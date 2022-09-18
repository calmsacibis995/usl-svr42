/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.file	"copy.s"

	.ident	"@(#)uts-x86:mem/copy.s	1.4"
	.ident	"$Header: $"

/	High-speed copy routines.

	.set	NBPW, 4		/ no of bytes in a word.

	.text

/
/	void bzero(caddr_t addr, size_t len);
/
/	Zeroes out a block of memory.
/	Arguments are the starting address and length.
/
/	struct_zero() was added to catch any references to the fast inline
/	structure zeroing routine that could not be put inline.  This
/	might occur if a file could not include inline.h for some reason.
/

	.set	ZEROADDR, 8
	.set	ZEROCOUNT, 12

	.align	4
	.globl	bzero
	.type	bzero,@function
	.globl	struct_zero
	.type	struct_zero,@function
bzero:
struct_zero:
	pushl	%ebp			/ save stack base
	movl	%esp, %ebp		/ set new stack base
	pushl	%edi			/ save %edi

	movl	ZEROADDR(%ebp), %edi	/ %edi <- address of bytes to clear

	movl	$0, %eax		/ sstol val
	movl	ZEROCOUNT(%ebp), %ecx	/ get size in bytes
	cmpl	%eax, %ecx		/ short circuit if len = 0
	jz	.bzdone
	shrl	$2, %ecx		/ count of double words to zero
	repz
	sstol				/ %ecx contains words to clear(%eax=0)
	movl	ZEROCOUNT(%ebp), %ecx	/ get size in bytes
	andl	$3, %ecx		/ do mod 4
	repz
	sstob				/ %ecx contains residual bytes to clear
.bzdone:
	popl	%edi
	popl	%ebp
	ret
	.size	bzero,.-bzero
	.size	struct_zero,.-struct_zero


/
/	void bcopy(caddr_t from, caddr_t to, size_t bytes);
/
/	Copy an arbitrary amount of arbitrarily-aligned data.
/	This is an optimized equivalent of the following C code:
/
/		void
/		bcopy(caddr_t from, caddr_t to, size_t bytes)
/		{
/			while (bytes--)
/				*to++ = *from++;
/		}
/
/	Based on the C library memcpy routine.
/
/	Replaces an older version which attempted to improve
/	performance by assuring long-word alignment before copying.
/	However, the cost of doing the alignment tends to swamp
/	the benefits.  It also seems that most copies in the kernel
/	are either small or involve at least one aligned argument.
/
	.set	FROMADR, 8		/ source address
	.set	TOADR, 12		/ destination address
	.set	BCOUNT, 16		/ count of bytes to copy

	.align	4
	.globl	bcopy
	.type	bcopy,@function
bcopy:
	pushl	%ebp
	movl	%esp, %ebp		/ setup stack frame
	pushl	%esi
	pushl	%edi			/ save registers
	movl	BCOUNT(%ebp), %ecx
	movl	FROMADR(%ebp), %esi	/ get source address
	movl	TOADR(%ebp), %edi	/ get destination address
	movl	%ecx, %eax		/ get count of bytes to copy
	andl	$NBPW-1, %eax
	shrl	$2, %ecx		/ convert to count of words to copy
	rep
	smovl				/ copy words
	xchgl	%eax, %ecx		/ copy remaining bytes, and load
					/ zero in to %eax for return value
	rep
	smovb
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
	.size	bcopy,.-bcopy

/
/	int bcmp(caddr_t buf1, caddr_t buf2, size_t bytes);
/
/	Compare two byte streams.  Returns 0 if they are identical, 1
/	if not.
/
/	It is an optimized equivalent of the following C code:
/
/		int
/		bcmp(caddr_t buf1, caddr_t buf2, size_t bytes)
/		{
/			while (bytes--)
/				if (*buf1++ != *buf2++)
/					return 1;
/			return 0;
/		}
/
/	Based on the C library memcmp routine.
/
	.align	4
	.globl	bcmp
	.type	bcmp,@function
bcmp:
	pushl	%ebp
	movl	%esp, %ebp		/ setup stack frame
	pushl	%esi
	pushl	%edi			/ save registers
	movl	BCOUNT(%ebp), %ecx
	movl	FROMADR(%ebp), %esi	/ get source address
	movl	TOADR(%ebp), %edi	/ get destination address
	movl	%ecx, %eax		/ get count of bytes to copy
	andl	$NBPW-1, %eax		/ %eax = count % NBPW
	shrl	$2, %ecx		/ %ecx = word count
/ ASSERT (%ecx == 0) or (ZF == 0) 
	repe
	scmpl				/ compare words
/ ASSERT (%ecx == 0) or (ZF == 0) 
	jne	bcmp_ret
	movl	%eax, %ecx		/ %ecx = remaining bytes
	repe
	scmpb				/ compare remaining bytes
bcmp_ret:
	setne	%al			/ setne will set %al to 0 or 1
					/ depending on the state of the
					/ condition code.  Note that the
					/ upper three bytes of %eax are
					/ already 0, since %eax is less
					/ than NBPW (4) based on previous
					/ computation.  Thus the setne assures
					/ that the return value in %eax is
					/ either 0 or 1.
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
	.size	bcmp,.-bcmp



/	int spath(caddr_t from, caddr_t to, size_t maxbufsize);
/
/	Get a pathname from system space into a caller-supplied buffer.
/	Returns -2 if pathname is too long, otherwise returns
/	the pathname length.

	.set	FROM, 8
	.set	TO, 12
	.set	MAXBUFSIZE, 16

	.align	4
	.globl	spath
	.type	spath,@function
spath:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	
	movl	FROM(%ebp), %esi	/ calculate the pathname length
	movl	%esi, %edi		/ %edi = %esi = start of string
	movb	$0, %al
	movl	MAXBUFSIZE(%ebp), %ecx
	incl	%ecx			/ make sure we scan long enough
	repnz
	scab				/ scan for null terminator
	decl	%edi			/ back up to null (if it's there)

	movl	%edi, %ecx
	subl	%esi, %ecx		/ compare the pathname length
	movl	MAXBUFSIZE(%ebp), %edi	/ with the maximum size of 
	cmpl	%ecx, %edi		/ the buffer
	jbe	.splenerr		/ error, if the length is 
					/ > (maxbufsize-1), -1 for NULL

	movl	%ecx, %eax		/ return length in %eax
	incl	%ecx			/ must copy null
	movl	TO(%ebp), %edi		/ destination
	repz
	smovb

.spreturn:
	popl	%edi
	popl	%esi
	popl	%ebp
	ret

.splenerr:
	movl	$-2, %eax		/ return error (-2) on pathname
					/ length error 
	jmp	.spreturn

	.size	spath,.-spath
