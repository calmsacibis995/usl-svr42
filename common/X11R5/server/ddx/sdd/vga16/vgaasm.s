/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.file	"vgaasm.s"
	.ident	"@(#)vga16:vga16/vgaasm.s	1.2"
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


#include "vtdefs.h"

#define MAP_MASK 	2
#define READ_MASK	4

	.text
	.globl	vga_set1
	.globl	vga_setn
	.globl	vga_solidn
	.globl	vga_stpl1
	.globl	vga_stpl2
	.globl	vga_stpl3
	.globl	vga_alcpy_one
	.globl	vga_alcpy_middle_up
	.globl	vga_alcpy_middle_down
	.globl	vga_bres_high
	.globl	vga_bres_low
	.globl	vga_shift_out
	.globl	vga_shift_out1
	.globl	vga_shift_outn
	.globl	vga_shiftflip
	.globl	vga_save_blot
	.globl	vga_restore_blot
	.globl	vga_opqstplblt_middle
	.globl	vga_copyopqstplblt_middle
	.globl	vga_stplblt_middle
	.globl	vga_put_points
	.globl	vga_stosb
	.globl	vga_copy_tile
	.globl	vga_write1
	.globl	vga_rotate
	.globl	vga_invertRotate
	.globl	vga_shiftl

/
/ external data referenced
/
	.globl	vga_bitflip
	.globl	vga_slbytes
	.globl	vga_write_map
	.globl	vga_read_map
	.globl	vt_allplanes
	.globl	gr_mode
	.globl	vga_blotted_addr


/
/
/	vga_set1(dst, cnt)	-- set 1 byte in each of cnt scanlines
/
/	Input:
/		BYTE	*dst	-- destination on screen
/		int	cnt	-- number of scanlines to set
/
vga_set1:
	MCOUNT			/ for profiling
	movl	4(%esp), %edx	/ destination
	movl	8(%esp), %ecx	/ count
	movl	vga_slbytes, %eax

another1:
	orb	$1, (%edx)		/ set byte
	addl	%eax, %edx		/ increment to next scanline
	decl	%ecx
	jnz	another1
	ret


/
/
/	vga_setn(dst, n, cnt)	-- set n bytes in each of cnt scanlines
/
/	Input:
/		BYTE	*dst	-- destination on screen
/		int	n	-- number of bytes per scanline to set
/		int	cnt	-- number of scanlines to set
/
vga_setn:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %esi		/ destination
	movl	20(%esp), %ebx		/ xcount
	movl	24(%esp), %edx		/ y count
	movl	vga_slbytes, %eax	/ set up increment to next scanline
	subl	%ebx, %eax

anothern:
	movl	%esi, %edi
	movl	%ebx, %ecx
	rep ;	movsb
	addl	%eax, %esi		/ increment to next scanline
	decl	%edx
	jnz	anothern		/ do another scanline

	popl	%ebx
	popl	%esi
	popl	%edi
	ret



/
/
/	vga_solidn(dst, n, cnt)	-- set n bytes in each of cnt scanlines
/				rasterop = GXCopy
/
/	Input:
/		BYTE	*dst	-- destination on screen
/		int	n	-- number of bytes per scanline to set
/		int	cnt	-- number of scanlines to set
/
vga_solidn:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi		/ destination
	movl	16(%esp), %ebx		/ xcount
	movl	20(%esp), %edx		/ y count
	movl	vga_slbytes, %eax	/ set up increment to next scanline
	subl	%ebx, %eax

another_solidn:
	movl	%ebx, %ecx
	rep ;	stosb
	addl	%eax, %edi		/ increment to next scanline
	decl	%edx
	jnz	another_solidn		/ do another scanline

	popl	%ebx
	popl	%edi
	ret


/
/
/	vga_stpl1(src, dst, h, shift, mask) -- stipple 1 byte in h scanlines
/					at dst.  Source for stipple is
/					in src (padded to 32 bits).  
/
/	Input:
/		BYTE	*src	-- source bits for stipple
/		BYTE	*dst	-- destination on screen
/		int	h	-- number of scanlines to set
/		BYTE	shift	-- count for left shift of stipple
/ 		BITS32	mask	-- mask of bits to paint
/
vga_stpl1:
	MCOUNT			/ for profiling
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi	/ source for stipple
	movl	24(%esp), %edi	/ destination
	movb	28(%esp), %bh	/ height
	movb	32(%esp), %cl	/ shift count
	movl	36(%esp), %ebp	/ mask
	movl	vga_slbytes, %edx

another_stpl1:
	lodsl				/ get stipple
	andl	%ebp, %eax		/ mask bits
	jz	next_stpl1_line		/ skip blank scanlines
	movb	(%edi), %ch		/ load latches
	shll	%cl, %eax		/ shift data
	movb	vga_bitflip(%eax), %ah
	movb	%ah, (%edi)		/ set bits

next_stpl1_line:
	addl	%edx, %edi
	decb	%bh
	jnz	another_stpl1

	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret


/
/
/   vga_stpl2(src, dst, h, shift, mask)	-- stipple 2 byte in h scanlines
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
/ 		BITS32	mask	-- mask of bits to paint
/
vga_stpl2:
	MCOUNT			/ for profiling
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi	/ source for stipple
	movl	24(%esp), %edi	/ destination
	movb	28(%esp), %ch	/ height
	movb	32(%esp), %cl	/ shift count
	movl	36(%esp), %ebp	/ mask
	movl	vga_slbytes, %edx
	movl	$0, %ebx

another_stpl2:
	lodsl				/ get stipple
	andl	%ebp, %eax		/ mask bits
	jz	next_stpl2_line

	movb	(%edi), %bl		/ load latches
	shll	%cl, %eax		/ shift it

	movb	%al, %bl
	movb	vga_bitflip(%ebx), %al
	movb	%al, (%edi)		/ write first byte

	movb	1(%edi), %bl		/ load latches
	movb	%ah, %bl
	movb	vga_bitflip(%ebx), %al
	movb	%al, 1(%edi)		/ write second byte

next_stpl2_line:
	addl	%edx, %edi
	decb	%ch			/ decrement scanline count
	jnz	another_stpl2

	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

	
/
/
/   vga_stpl3(src, dst, h, shift) -- stipple 3 bytes in h scanlines at dst.  
/				Source for stipple is in src (padded to 
/				32 bits). Stipple is drawn three times in 
/				successive locations in each scanline.
/
/	Input:
/		BYTE	*src	-- source bits for stipple
/		BYTE	*dst	-- destination on screen
/		int	h	-- number of scanlines to set
/		BYTE	shift	-- count for right shift of stipple
/ 		BITS32	mask	-- mask of bits to paint
/
vga_stpl3:
	MCOUNT			/ for profiling
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi	/ source for stipple
	movl	24(%esp), %edi	/ destination
	movb	28(%esp), %ch	/ height
	movb	32(%esp), %cl	/ shift count
	movl	36(%esp), %ebp	/ mask
	movl	vga_slbytes, %edx
	movl	$0, %ebx

another_stpl3:
	lodsl				/ get stipple
	andl	%ebp, %eax		/ mask bits
	jz	next_stpl3_line

	movb	(%edi), %bl		/ load latches
	shll	%cl, %eax		/ shift it

	movb	%al, %bl
	movb	vga_bitflip(%ebx), %al
	movb	%al, (%edi)		/ write first byte

	movb	1(%edi), %al		/ load latches
	movb	%ah, %bl		/ get second byte
	movb	vga_bitflip(%ebx), %al
	movb	%al, 1(%edi)		/ write second byte

	movb	2(%edi), %al		/ load latches
	shrl	$16, %eax		/ get third byte
	movb	vga_bitflip(%eax), %al
	movb	%al, 2(%edi)		/ write last byte

next_stpl3_line:
	addl	%edx, %edi
	decb	%ch			/ decrement scanline count
	jnz	another_stpl3

	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

	

/
/
/ 	vga_alcpy_one(psrc, pdst, ycnt, mask, incr, planes)
/ 		-- copy ycnt bytes from from psrc to pdst using the mask
/		specified.  Do this a plane at a time.
/ 
/ 	Input:
/ 		BYTE	*psrc	-- pointer to source data
/ 		BYTE	*pdst	-- pointer to destination
/ 		int	ycnt	-- number of lines to copy
/		BYTE	mask	-- bitmask for bytes copied
/		int	incr	-- amount to increment psrc and pdst by
/		BYTE	planes	-- number of planes to copy.
/
vga_alcpy_one:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	32(%esp), %ebx		/ set up increment to next scanline
	movb	36(%esp), %ch		/ number of planes
	movb	$0, %cl

	movw	$VGA_GRAPH, %dx		/ set up bitmask
	movb	$BITMASK, %al
	movb	28(%esp), %ah
	outw	(%dx)		

another_plane:
	cmpb	%ch, %cl
	jz	done_one

	movw	$VGA_SEQ, %dx		/ set up write mask
	movl	$0, %eax
	movb	%cl, %al
	addl	vga_write_map, %eax
	movb	(%eax), %ah
	movb	$MAP_MASK, %al
	outw	(%dx)		

	movw	$VGA_GRAPH, %dx		/ set up read mask
	movl	$0, %eax
	movb	%cl, %al
	addl	vga_read_map, %eax
	movb	(%eax), %ah
	movb	$READ_MASK, %al
	outw	(%dx)		

	movl	16(%esp), %esi		/ src
	movl	20(%esp), %edi		/ dst
	movl	24(%esp), %edx		/ ycnt

another_one:				/ copy data
	movb	(%esi), %ah		/ load a byte from a plane
	movb	(%edi), %al		/ load the latches at the dst
	movb	%ah, (%edi)		/ update a plane
	addl	%ebx, %esi		/ go to next scanline
	addl	%ebx, %edi
	decl	%edx
	jnz	another_one

	incb	%cl
	jmp	another_plane

done_one:
	popl	%ebx
	popl	%esi
	popl	%edi
	ret


/
/
/ 	vga_alcpy_middle(psrc, pdst, xcnt, ycnt)
/ 		-- copy ycnt lines of xcnt bytes from psrc to pdst.
/ 		write mode one has already been set up, so we are
/ 		doing a fast copy using the VGA latches.
/ 
/ 	Input:
/ 		BYTE	*psrc	-- pointer to source data
/ 		BYTE	*pdst	-- pointer to destination
/ 		int	xcnt	-- number of bytes to copy
/ 		int	ycnt	-- number of lines to copy
/
vga_alcpy_middle_up:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %esi		/ src
	movl	20(%esp), %edi		/ dst
	movl	24(%esp), %edx		/ xcnt
	movl	28(%esp), %ebx		/ ycnt
	movl	vga_slbytes, %eax	/ set up increment to next scanline
	subl	%edx, %eax

another_up:
	movl	%edx, %ecx

	rep ;	movsb

	addl	%eax, %esi		/ increment to next scanline
	addl	%eax, %edi		/ increment to next scanline
	decl	%ebx
	jnz	another_up

	popl	%ebx
	popl	%esi
	popl	%edi
	ret


vga_alcpy_middle_down:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %esi		/ src
	movl	20(%esp), %edi		/ dst
	movl	24(%esp), %edx		/ xcnt
	movl	28(%esp), %ebx		/ ycnt
	movl	vga_slbytes, %eax	/ set up increment to next scanline
	addl	%edx, %eax

another_down:
	movl	%edx, %ecx

	rep ;	movsb

	subl	%eax, %esi		/ increment to next scanline
	subl	%eax, %edi		/ increment to next scanline
	decl	%ebx
	jnz	another_down

	popl	%ebx
	popl	%esi
	popl	%edi
	ret


/
/
/ 	vga_bres_high(paddr, mask, cnt, inc1, inc2, incy, d)
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
vga_bres_high:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %edi		/ destination
	movw	20(%esp), %ax		/ initial mask
	movl	24(%esp), %ecx		/ count
	movl	32(%esp), %edx		/ incr2
	movl	36(%esp), %ebx		/ yincr
	movl	40(%esp), %esi		/ current delta 

another_hbres:
	addl	%ebx, %edi		/ update y coordinate 

	cmpl	$0, %esi
	jge	hbres_nextbyte
	addl	%edx, %esi		/ add incr2 to delta

hbres_setpix:
	movb	(%edi), %al		/ load latches
	movb	%ah, (%edi)		/ write the byte

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
/	vga_bres_low(paddr, mask, cnt, inc1, inc2, incy, d)
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
vga_bres_low:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %edi		/ destination
	movw	20(%esp), %bx		/ initial mask
	movl	24(%esp), %ecx		/ count
	movl	32(%esp), %edx		/ incr2
	movl	40(%esp), %esi		/ current delta 

another_lbres:
	cmpl	$0, %esi
	jge	lbres_nextline
	addl	%edx, %esi		/ increment delta by incr2

lbres_checkbyte:
	shrb	%bh			/ do next bit in byte
	jz	lbres_nextbyte
	orb	%bh, %bl		/ just update bits

lbres_nextbit:
	decl	%ecx
	jnz	another_lbres

	movb	(%edi), %al		/ load latches
	movb	%bl, (%edi)		/ get last byte drawn

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

lbres_nextline:
	movb	(%edi), %al		/ load latches
	movb	%bl, (%edi)		/ time to switch to a new line
	movb	$0, %bl			/ so draw the current bits
	addl	36(%esp), %edi		/ increment to next scanline
	addl	28(%esp), %esi		/ increment delta by incr1
	jmp	lbres_checkbyte

lbres_nextbyte:
	movb	(%edi), %al		/ load latches
	movb	%bl, (%edi)		/ time to switch to next byte
	incl	%edi			/ so draw the current bits
	movb	$0x80, %bh		/ set a new mask
	movb	%bh, %bl
	jmp	lbres_nextbit


/
/
/	vga_shift_out(psrc, pdst, cnt, startmask, endmask, shift)
/		-- Special case of bitblt where the ROP is copy.  We start
/		with a scanline of pixels, then do the shifts and writes
/		all at once for speed.
/
/	Input:
/		BYTE	*psrc		-- pointer to source data
/		BYTE	*pdst		-- pointer to destination
/		int	cnt		-- number of bytes to copy 
/		BYTE	startmask	-- mask for first byte of each line
/		BYTE	endmask		-- mask for last byte of each line
/		int	shift		-- number of bits to shift left by
/
vga_shift_out:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %esi	/ source 
	movl	20(%esp), %edi	/ destination
	movl	24(%esp), %ebx	/ cnt
	movb	28(%esp), %ah	/ start mask
	movb	36(%esp), %cl	/ rotate count
	movw	$VGA_GRAPH, %dx

	cmpb	$0, %ah		/ if there is a start mask, do the first byte
	jz	final_out	
	movb	$BITMASK, %al	/ use BITMASK reg
	outw	(%dx)		
	lodsw			/ al = first byte, ah = second byte
	decl	%esi		/ point to second byte
	rolw	%cl, %ax	/ rotate data
	movb	(%edi), %ah	/ load latches
	movb	%al, (%edi)	/ write data out
	incl	%edi		/ increment destination
	decl	%ebx
	
middle_out:
	movb	$BITMASK, %al	/ use BITMASK reg
	movb	$0xff, %ah	/ all bits on
	outw	(%dx)		

another_out:
	cmpl	$0, %ebx	/ do count middle bytes
	jz	final_out
	
	lodsw			/ al = first byte, ah = second byte
	decl	%esi		/ point to second byte
	rolw	%cl, %ax	/ rotate data
	stosb			/ write data out
	decl	%ebx
	jmp	another_out

final_out:
	movb	32(%esp), %ah	/ end mask
	movb	$BITMASK, %al	/ use BITMASK reg
	outw	(%dx)		
	lodsw			/ al = first byte, ah = second byte
	rolw	%cl, %ax	/ rotate data
	movb	(%edi), %ah	/ load latches
	movb	%al, (%edi)	/ write data out

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

/
/
/	vga_shift_out1(psrc, pdst, srcinc, cnt, shift)
/		-- Special case of bitblt where the ROP is copy.  This
/		draws one byte (8 pixels) of output and used for the
/		edges of a filled area that are masked
/
/	Input:
/		BYTE	*psrc		-- pointer to source data
/		BYTE	*pdst		-- pointer to destination
/		int	srcinc		-- distance between source scanlines
/		int	cnt		-- number of lines to draw
/		int	shift		-- number of bits to shift left by
/
vga_shift_out1:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %esi	/ source 
	movl	20(%esp), %edi	/ destination
	movl	24(%esp), %ebx	/ srcinc
	movl	28(%esp), %edx	/ cnt
	movb	32(%esp), %cl	/ rotate count

another_shift_out1:
	movb	(%edi), %ah	/ load latches
	movw	(%esi), %ax	/ al = first byte, ah = second byte
	rolw	%cl, %ax	/ rotate data
	movb	%al, (%edi)	/ write data out
	addl	vga_slbytes, %edi
	addl	%ebx, %esi
	decl	%edx
	jnz another_shift_out1
	
	popl	%ebx
	popl	%esi
	popl	%edi
	ret

/
/
/	vga_shift_outn(psrc, pdst, srcinc, xcnt, ycnt, shift)
/		-- Special case of bitblt where the ROP is copy.  This
/		draws the middle (whole) bytes in a fill operation.
/
/	Input:
/		BYTE	*psrc		-- pointer to source data
/		BYTE	*pdst		-- pointer to destination
/		int	srcinc		-- distance between source scanlines
/		int	xcnt		-- number of bytes to draw
/		int	ycnt		-- number of lines to draw
/		int	shift		-- number of bits to shift left by
/
vga_shift_outn:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %esi	/ source 
	movl	20(%esp), %edi	/ destination
	movl	28(%esp), %ebx	/ xcnt
	movb	36(%esp), %cl	/ rotate count
	subl	%ebx, 24(%esp)	/ set up real source increment
	movl	vga_slbytes, %edx
	subl	%ebx, %edx	/ set up real destination increment

another_shift_line:
	movl	28(%esp), %ebx	/ xcnt
	
another_shift_outn:
	lodsw			/ al = first byte, ah = second byte
	decl	%esi		/ point to second byte
	rolw	%cl, %ax	/ rotate data
	stosb			/ write data out
	decl	%ebx
	jnz another_shift_outn

	addl	24(%esp), %esi	/ increment source
	addl	%edx, %edi	/ increment dest
	decl	32(%esp)
	jnz another_shift_line

	popl	%ebx
	popl	%esi
	popl	%edi
	ret



/
/
/	vga_shiftflip(src, dst, shift, cnt, pad)	-- copy src bytes to 
/						dst shifting by shift and 
/						flipping the bits for cnt bytes.
/
/	Input:
/		BYTE	*src	-- source bits 
/		BYTE	*dst	-- destination
/		int	shift	-- number of bits to shift by
/		BYTE	cnt	-- count for number of bytes to copy
/		BYTE	pad	-- if non-zero, we will need an extra byte in dst
/
vga_shiftflip:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %esi	/ source for stipple
	movl	20(%esp), %edi	/ destination
	movb	24(%esp), %cl	/ shift amount
	movb	28(%esp), %bl	/ byte count
	movl	$0, %eax

	cmpl	$0, 32(%esp)	/ padding needed?
	je	another_shift	/ no just get first byte
	movb	$0, %al		
	movb	(%esi), %ah	/ get first byte
	shrw	%cl, %ax
	movb	$0, %ah
	movb	vga_bitflip(%eax), %al  / flip bits around
	stosb

another_shift:
	cmpb	$0, %bl		/ do cnt bytes
	jz	last_shift

	lodsw			/ al = first byte, ah = second byte
	decl	%esi		/ point to second byte
	shrw	%cl, %ax	/ shift data
	movb	$0, %ah
	movb	vga_bitflip(%eax), %al  / flip bits around
	stosb			/ write shifted byte out
	decl	%ebx
	jmp	another_shift

last_shift:
	lodsb			/ get last byte
	shrb	%cl, %al	/ shift it
	movb	$0, %ah
	movb	vga_bitflip(%eax), %al  / flip bits around
	stosb			/ write it out

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

	
/
/
/   vga_save_blot(addr, w, h) 	-- save the area blotted out by the cursor
/
/	Input:
/		BYTE	*addr	-- address to put the data
/		int	w	-- the number of bytes per line
/		int	h	-- the number of lines
/
vga_save_blot:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %edi		/ destination address
	movl	20(%esp), %ecx		/ width
	movl	24(%esp), %ebx		/ height
	movl	vga_blotted_addr, %esi	/ source address
	movl	vga_slbytes, %eax
	subl	%ecx, %eax		/ set up increment to next line

another_save_blot:
	movl	20(%esp), %ecx		/ width
	rep ; 	movsb			/ copy a lines worth of data
	addl	%eax, %esi		/ increment to next line
	decl	%ebx
	jnz	another_save_blot	/ go do another line
	
	popl	%ebx
	popl	%esi
	popl	%edi
	ret

/
/
/   vga_restore_blot(addr, w, h) -- restore the area blotted out by the cursor
/
/	Input:
/		BYTE	*addr	-- address to get data
/		int	w	-- the number of bytes per line
/		int	h	-- the number of lines
/
vga_restore_blot:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %esi		/ source address
	movl	20(%esp), %ecx		/ width
	movl	24(%esp), %ebx		/ height
	movl	vga_blotted_addr, %edi	/ destination address
	movl	vga_slbytes, %eax
	subl	%ecx, %eax		/ set up increment to next line

another_restore_blot:
	movl	20(%esp), %ecx		/ width
	rep ; 	movsb			/ copy a lines worth of data
	addl	%eax, %edi		/ increment to next line
	decl	%ebx
	jnz	another_restore_blot	/ go do another line
	
	popl	%ebx
	popl	%esi
	popl	%edi
	ret

/
/ vga_opqstplblt_middle(src, dst, cnt, fg, bg)	-- do an opaque stipple 
/
/ Input:
/ 	BYTE *src		-- points to source bits
/ 	BYTE *dst		-- points to destination data
/ 	BYTE cnt		-- number of bytes to stipple
/ 	BYTE fg, bg		-- foreground and background colors
/
vga_opqstplblt_middle:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi

	movw	$VGA_GRAPH+1, %dx
	movb    24(%esp), %al		/ foreground
	outb	(%dx)

	movl	12(%esp), %esi		/ source for stipple
	movl	16(%esp), %edi		/ destination
	movb    20(%esp), %cl		/ count

another_fgstpl:
					/ do foreground
	movb	(%edi), %ch		/ load latches
	movsb				/ store foreground color
	decb	%cl
	jnz	another_fgstpl

	movb    28(%esp), %al		/ background
	outb	(%dx)

	movl	12(%esp), %esi		/ source for stipple
	movl	16(%esp), %edi		/ destination
	movb    20(%esp), %cl		/ count

another_bgstpl:
	movb	(%edi), %ch		/ load latches
	lodsb				/ get a byte of stipple bitmap
	notb	%al			/ invert stipple
	stosb				/ store background color
	decb	%cl
	jnz	another_bgstpl

	popl	%esi
	popl	%edi
	ret


/
/ vga_copyopqstplblt_middle(src, dst, cnt) -- do an opaque stipple by using
/					      the background color stored in
/					      the latches and the foreground
/					      color stippled using write mode
/					      3.  
/
/ Input:
/ 	BYTE *src		-- points to source bits
/ 	BYTE *dst		-- points to destination data
/ 	BYTE cnt		-- number of bytes to stipple
/
vga_copyopqstplblt_middle:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	movl	12(%esp), %esi		/ source for stipple
	movl	16(%esp), %edi		/ destination
	movl    20(%esp), %ecx		/ count

	rep;	movsb			/ draw the bits

	popl	%esi
	popl	%edi
	ret

/
/ vga_stplblt_middle(src, dst, cnt, fg)	-- do a stipple using write mode
/					3.  
/
/ Input:
/ 	BYTE *src		-- points to source bits
/ 	BYTE *dst		-- points to destination data
/ 	BYTE cnt		-- number of bytes to stipple
/	BYTE fg			-- color to stipple
/
vga_stplblt_middle:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	movl	12(%esp), %esi		/ source for stipple
	movl	16(%esp), %edi		/ destination
	movb    20(%esp), %cl		/ count

	movw	$VGA_GRAPH+1, %dx
	movb    24(%esp), %al		/ foreground
	outb	(%dx)

another_stpl:
	movb	(%edi), %ch		/ load latches
	movsb				/ store foreground color
	decb	%cl
	jnz	another_stpl

	popl	%esi
	popl	%edi
	ret


/
/ vga_put_points(pts, dst, cnt)	-- draw a series of points based on the list
/				of (x, y) values passed in.
/
/ Input:
/ 	DDXPointRec *pts	-- points to source bits
/	BYTE	    *dst	-- pointer to beginning of frame buffer
/ 	int cnt			-- number of bytes to stipple
/
vga_put_points:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp), %esi		/ pointer to points
	movl    24(%esp), %edi		/ count
	movw	$VGA_GRAPH+1, %dx

another_point:
	movswl	2(%esi), %eax		/ get y position of point
	imull	vga_slbytes, %eax	/ multiply by number of bytes in line
	movswl	(%esi), %ebx		/ get x position of point
	movb	%bl, %cl		/ save a copy
	shrl	$3, %ebx		/ get byte offset
	addl	%eax, %ebx		/ add the y byte offset
	addl	20(%esp), %ebx		/ get final virtual address for dot

	andb	$7, %cl			/ get bit offset in byte
	movb	$0x80, %al
	shrb	%cl, %al
	
	outb	(%dx)			/ set mask
	orb	$1, (%ebx)		/ set dot

	addl	$4, %esi
	decl	%edi
	jnz	another_point

	popl	%ebx
	popl	%esi
	popl	%edi
	ret


/
/ vga_stosb(dst, end, cnt, inc) -- Starting at dst, draw cnt bytes per line, 
/				incrementing by inc after each line.  Finish
/				when dst > end.
/
/ Input:
/	BYTE	*dst		-- pointer to beginning of drawing
/	BYTE	*end		-- pointer to end of drawing
/ 	int	cnt		-- number of bytes to draw per line
/	int	inc		-- amount to increment to next line
/
vga_stosb:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	movl	12(%esp), %edi		/ pointer to destination
	movl	16(%esp), %eax		/ pointer to last position
	movl	24(%esp), %edx		/ increment amount

another_stosb:
	movl    20(%esp), %ecx		/ count
	rep;	stosb			/ draw a line
	addl	%edx, %edi		/ increment to next line
	cmpl	%edi, %eax		/ dont go past the end
	jg	another_stosb

	popl	%esi
	popl	%edi
	ret


/
/ vga_copy_tile(dst, end, cnt, inc) -- Starting at dst, draw cnt bytes per 
/				line, skipping every other destination byte.
/				Increment by inc after each line.  Finish
/				when dst > end.
/
/ Input:
/	BYTE	*dst		-- pointer to beginning of drawing
/	BYTE	*end		-- pointer to end of drawing
/ 	int	cnt		-- number of bytes to draw per line
/	int	inc		-- amount to increment to next line
/
vga_copy_tile:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	movl	12(%esp), %edi		/ pointer to destination
	movl	16(%esp), %eax		/ pointer to last position
	movl	24(%esp), %edx		/ increment amount

another_copy_tile:
	movl    20(%esp), %ecx		/ count

another_tile_byte:
	movb	%al, (%edi)		/ source is in latches
	addl	$2, %edi
	decl	%ecx
	jg	another_tile_byte

	addl	%edx, %edi		/ increment to next line
	cmpl	%edi, %eax		/ dont go past the end
	jg	another_copy_tile

	popl	%esi
	popl	%edi
	ret


/
/ vga_write1(src, dst, end, inc) -- Starting at dst, draw one byte per line, 
/			       incrementing by inc after each line.  Finish
/			       when dst > end.
/
/ Input:
/	BYTE	src		-- source data to write
/	BYTE	*dst		-- pointer to beginning of drawing
/	BYTE	*end		-- pointer to end of drawing
/	int	inc		-- amount to increment to next line
/
vga_write1:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	movb	12(%esp), %al		/ get source data
	movl	16(%esp), %edi		/ pointer to destination
	movl	20(%esp), %ecx		/ pointer to last position
	movl	24(%esp), %edx		/ increment amount

another_write1:
	movb	(%edi), %ah
	movb	%al, (%edi)
	addl	%edx, %edi		/ increment to next line
	cmpl	%edi, %ecx		/ dont go past the end
	jg	another_write1

	popl	%esi
	popl	%edi
	ret

/
/ vga_rotate (src, dst, cnt, shift) -- rotate cnt bytes from src left by
/					 shift bits.  Spans byte boundries.
/
/ input:
/	BYTE	*src		-- pointer to source
/	BYTE	*dst		-- pointer to destination
/	int	cnt		-- number of bytes to shift
/	int	shift		-- number of bits to shift
/
vga_rotate:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	movl	12(%esp), %esi
	movl	16(%esp), %edi
	movl	20(%esp), %edx
	movl	24(%esp), %ecx

rotate_loop:
	lodsb
	movb	(%esi), %ah
	rolw	%cl, %ax
	stosb
	dec	%edx
	jg	rotate_loop

	popl	%esi
	popl	%edi
	ret

/
/ vga_invertShift (src, dst, cnt, shift) -- rotate cnt bytes from src left by
/					shift bits.  Spans byte boundries.
/					Logically NOT bits before writing
/
/ input:
/	BYTE	*src		-- pointer to source
/	BYTE	*dst		-- pointer to destination
/	int	cnt		-- number of bytes to shift
/	int	shift		-- number of bits to shift
/
vga_invertRotate:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	movl	12(%esp), %esi
	movl	16(%esp), %edi
	movl	20(%esp), %edx
	movl	24(%esp), %ecx

irotate_loop:
	lodsb
	movb	(%esi), %ah
	rolw	%cl, %ax
	notb	%al
	stosb
	dec	%edx
	jg	irotate_loop

	popl	%esi
	popl	%edi
	ret

/
/ vga_shiftl (src, dst, cnt, shift) -- shift cnt bytes from src left by
/					 shift bits.  Spans byte boundries.
/
/ input:
/	BYTE	*src		-- pointer to source
/	BYTE	*dst		-- pointer to destination
/	int	cnt		-- number of bytes to shift
/	int	shift		-- number of bits to shift
/
vga_shiftl:
	MCOUNT				/ for profiling
	pushl	%edi
	pushl	%esi
	movl	12(%esp), %esi
	movl	16(%esp), %edi
	movl	20(%esp), %edx
	movl	24(%esp), %ecx

	/ Preserve the right-most bits of the first byte of the destination
	/ by shifting them into the high bits of al.  They will be shifted
	/ back into proper position with the source.
	movb	(%edi), %ah
	shrw	%cl, %ax
	movb	(%esi), %ah

shiftl_loop:
	shlw	%cl, %ax
	movb	%ah, (%edi)
	incl	%edi
	lodsb
	movb	(%esi), %ah
	dec	%edx
	jg	shiftl_loop

	popl	%esi
	popl	%edi
	ret
