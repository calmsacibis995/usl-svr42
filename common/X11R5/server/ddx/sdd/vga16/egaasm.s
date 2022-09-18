/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)vga16:vga16/egaasm.s	1.2"

/
/	Copyright (c) 1991 USL
/	All Rights Reserved 
/ 
/ 	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
/ 	The copyright notice above does not evidence any 
/ 	actual or intended publication of such source code.
/ 
/ 	Copyrighted as an unpublished work.
/ 	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
/ 	All rights reserved.
/

	.file	"egaasm.s"

#include "vtdefs.h"

	.text
	.globl	ega_stpl1
	.globl	ega_stpl2
	.globl	ega_stpl3
	.globl	ega_bres_high
	.globl	ega_bres_low
	.globl	ega_opqstplblt_middle
	.globl	ega_stplblt_middle

/
/ external data referenced
/
	.globl	vga_bitflip
	.globl	vga_slbytes


/
/
/	ega_stpl1(src, dst, h, shift, mask)	-- stipple 1 byte in h scanlines
/					at dst.  Source for stipple is
/					in src (padded to 32 bits).  
/
/	Input:
/		BYTE	*src	-- source bits for stipple
/		BYTE	*dst	-- destination on screen
/		int	h	-- number of scanlines to set
/		BYTE	shift	-- count for left shift of stipple
/		BITS32	mask	-- mask of bits to paint
/
ega_stpl1:
	MCOUNT			/ for profiling
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi	/ source for stipple
	movl	24(%esp), %edi	/ destination
	movb	28(%esp), %ch	/ height
	movb	32(%esp), %cl	/ rotate count
	movl	36(%esp), %ebp	/ mask
	movw	$VGA_GRAPH+1, %dx
	movl	vga_slbytes, %ebx

another_stpl1:
	lodsl				/ get stipple
	andl	%ebp, %eax
	jz	next_stpl1_line

	shll	%cl, %eax		/ shift data

	movb	vga_bitflip(%eax), %al
	movb	(%edi), %ah		/ load latches
	outb	(%dx)
	movb	$1, (%edi)		/ set data

next_stpl1_line:
	addl	%ebx, %edi
	decb	%ch
	jnz	another_stpl1

	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret


/
/
/   ega_stpl2(src, dst, h, shift, mask)	-- stipple 1 byte in h scanlines
/					at dst.  Source for stipple is
/					in src (padded to 32 bits).  
/					Stipple is drawn twice in 
/					successive locations in each
/					scanline.
/
/	Input:
/		BYTE	*src	-- source bits for stipple
/		BYTE	*dst	-- destination on screen
/		int	h	-- number of scanlines to set
/		BYTE	shift	-- count for left shift of stipple
/		BITS32	mask	-- mask of bits to paint
/
ega_stpl2:
	MCOUNT			/ for profiling
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi	/ source for stipple
	movl	24(%esp), %edi	/ destination
	movb	28(%esp), %ch	/ number of scanlines
	movb	32(%esp), %cl	/ rotate count
	movl	36(%esp), %ebp	/ mask
	movw	$VGA_GRAPH+1, %dx
	movl	$0, %ebx

another_stpl2:
	lodsl				/ get stipple
	andl	%ebp, %eax
	jz	next_stpl2_line

	shll	%cl, %eax		/ shift it

	movb	%al, %bl		/ draw first byte
	movb	vga_bitflip(%ebx), %al
	movb	(%edi), %bl		/ load latches
	outb	(%dx)
	movb	$1, (%edi)

	movb	%ah, %bl		/ draw second byte
	movb	vga_bitflip(%ebx), %al
	movb	1(%edi), %bl		/ load latches
	outb	(%dx)
	movb	$1, 1(%edi)

next_stpl2_line:
	addl	vga_slbytes, %edi
	decb	%ch		/ decrement scanline count
	jnz	another_stpl2

	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

	
/
/
/   ega_stpl3(src, dst, h, shift, mask) -- stipple 3 bytes in h scanlines at dst.  
/				Source for stipple is in src (padded to 
/				32 bits).  Stipple is drawn in successive 
/				locations in each scanline.
/
/	Input:
/		BYTE	*src	-- source bits for stipple
/		BYTE	*dst	-- destination on screen
/		int	h	-- number of scanlines to set
/		BYTE	shift	-- count for right shift of stipple
/		BITS32	mask	-- mask of bits to paint
/
ega_stpl3:
	MCOUNT			/ for profiling
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi	/ source for stipple
	movl	24(%esp), %edi	/ destination
	movb	28(%esp), %ch	/ scanline count
	movb	32(%esp), %cl	/ shift count
	movl	36(%esp), %ebp	/ mask
	movl	$0, %ebx
	movw	$VGA_GRAPH+1, %dx

another_stpl3:
	lodsl				/ get stipple
	andl	%ebp, %eax
	jz	next_stpl3_line
	shll	%cl, %eax		/ shift it

	movb	%al, %bl		/ do first byte
	movb	vga_bitflip(%ebx), %al
	movb	(%edi), %bl		/ load latches
	outb	(%dx)			/ set bitmask
	orb	$1, (%edi)

	movb	%ah, %bl		/ do second byte
	movb	vga_bitflip(%ebx), %al
	movb	1(%edi), %bl		/ load latches
	outb	(%dx)			/ set bitmask
	orb	$1, 1(%edi)

	shrl	$16, %eax		/ do third byte
	movb	vga_bitflip(%eax), %al
	movb	2(%edi), %bl		/ load latches
	outb	(%dx)			/ set bitmask
	orb	$1, 2(%edi)

next_stpl3_line:
	addl	vga_slbytes, %edi
	decb	%ch		/ decrement scanline count
	jnz	another_stpl3

	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret


/
/
/ 	ega_bres_high(paddr, mask, cnt, inc1, inc2, incy, d)
/ 					-- draw a one bit line starting at
/ 					address paddr for cnt points in the
/ 					the current linestyle with: 
/ 					|slope| > 1.  Points are drawn from
/ 					left to right.
/ 
/ 	Input:
/ 		BYTE *paddr		-- points to first pixel in line
/ 		unsigned short mask	-- mask of first pixel within a byte
/ 		short cnt		-- number of pixels to be painted
/ 		short inc1, inc2, incy  -- increments for edge walking
/ 		short d			-- current position along line
/
ega_bres_high:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %edi		/ destination
	movw	20(%esp), %ax		/ initial mask
	movl	24(%esp), %ecx		/ count
	movl	36(%esp), %ebx		/ yincr
	movl	40(%esp), %esi		/ current delta 

	movw	$VGA_GRAPH, %dx		/ GRAPHICS reg
	movb	$BITMASK, %al		/ use BITMASK reg

another_hbres:
	addl	%ebx, %edi		/ update y coordinate 

	cmpl	$0, %esi
	jge	hbres_nextbyte
	addl	32(%esp), %esi		/ add incr2 to delta

hbres_setpix:
	outw	(%dx)			/ set the pixel
	orb	$1, (%edi)

	decl	%ecx
	jnz	another_hbres

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

hbres_nextbyte:
	shrb	%ah			/ do next bit in byte
	jnz	hbres1
	movb	$0x80, %ah		/ set a new mask
	incl	%edi
hbres1:
	addl	28(%esp), %esi		/ add incr1 to delta
	jmp	hbres_setpix



/
/
/	ega_bres_low(paddr, mask, cnt, inc1, inc2, incy, d)
/					-- draw a one bit line starting at
/					address paddr for cnt points in the
/ 					the current linestyle with: 
/ 					|slope| <= 1.  Points are drawn from
/ 					left to right.
/ 
/ 	Input:
/ 		BYTE *paddr		-- points to first pixel in line
/ 		unsigned short mask	-- mask of first pixel within a byte
/ 		short cnt		-- number of pixels to be painted
/ 		short inc1, inc2, incy  -- increments for edge walking
/ 		short d			-- current position along line
/
ega_bres_low:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %edi		/ destination
	movw	20(%esp), %bx		/ initial mask
	movl	24(%esp), %ecx		/ count
	movl	40(%esp), %esi		/ current delta 

	movw	$VGA_GRAPH, %dx		/ GRAPHICS reg
	movw	$BITMASK, %ax		/ use BITMASK reg

another_lbres:
	cmpl	$0, %esi
	jge	lbres_nextline
	addl	32(%esp), %esi		/ increment delta by incr2

lbres_checkbyte:
	shrb	%bh			/ do next bit in byte
	jz	lbres_nextbyte
	orb	%bh, %ah		/ just update bits

lbres_nextbit:
	decl	%ecx
	jnz	another_lbres

	outw	(%dx)			/ get last byte drawn
	orb	$1, (%edi)

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

lbres_nextline:
	outw	(%dx)			/ time to switch to a new line
	orb	$1, (%edi)		/ so draw the current bits
	movb	$0, %ah
	addl	36(%esp), %edi		/ increment to next scanline
	addl	28(%esp), %esi		/ increment delta by incr1
	jmp	lbres_checkbyte

lbres_nextbyte:
	outw	(%dx)			/ time to switch to next byte
	orb	$1, (%edi)		/ so draw the current bits
	incl	%edi
	movb	$0x80, %bh		/ set a new mask
	movb	%bh, %ah
	jmp	lbres_nextbit


/
/ ega_opqstplblt_middle(src, dst, cnt, fg, bg)	-- do an opaque stipple 
/
/ Input:
/ 	BYTE *src		-- points to source bits
/ 	BYTE *dst		-- points to destination data
/ 	BYTE cnt		-- number of bytes to stipple
/ 	BYTE fg, bg		-- foreground and background colors
/
ega_opqstplblt_middle:
	MCOUNT				/ for profiling
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi		/ source for stipple
	movl	24(%esp), %edi		/ destination
	movb    28(%esp), %cl		/ count
	movb    32(%esp), %bl		/ foreground
	movb    36(%esp), %bh		/ background
	movw	$VGA_GRAPH+1, %dx

another_opqstpl:
					/ do foreground
	movb	(%edi), %ch		/ load latches
	lodsb				/ get a byte of stipple bitmap
	outb	(%dx)
	movb	%bl, (%edi)		/ store foreground color

					/ do background
	notb	%al			/ invert stipple
	movb	(%edi), %ch		/ load latches
	outb	(%dx)
	movb	%bh, (%edi)		/ store background color
	incl	%edi

	decb	%cl
	jnz	another_opqstpl

	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret


/
/ ega_stplblt_middle(src, dst, cnt, fg)	-- do a stipple 
/
/ Input:
/ 	BYTE *src		-- points to source bits
/ 	BYTE *dst		-- points to destination data
/ 	BYTE cnt		-- number of bytes to stipple
/ 	BYTE fg			-- color to stipple
/
ega_stplblt_middle:
	MCOUNT				/ for profiling
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi		/ source for stipple
	movl	24(%esp), %edi		/ destination
	movb    28(%esp), %cl		/ count
	movb    32(%esp), %bl		/ foreground
	movw	$VGA_GRAPH+1, %dx

another_stpl:
					/ do foreground
	movb	(%edi), %ch		/ load latches
	lodsb				/ get a byte of stipple bitmap
	outb	(%dx)
	movb	%bl, (%edi)		/ store foreground color
	incl	%edi

	decb	%cl
	jnz	another_stpl

	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
