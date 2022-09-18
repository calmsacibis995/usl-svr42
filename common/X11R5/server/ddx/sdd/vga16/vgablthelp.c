/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgablthelp.c	1.5"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vgaregs.h"
#include "vtio.h"
#include "vga.h"
#include "sys/inline.h"



/* 
 *	vga_slcopyin(psrc, w, byteshift)	-- copy 4 planes of bits from
 *						the screen position at psrc
 *						to vga_slbuf.
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to screen memory
 *		int	cnt		-- number of bytes to copy
 *		int	pad		-- amount to pad at the start
 */
vga_slcopyin(psrc, cnt, pad)
BYTE	*psrc;
register int	cnt, pad;
{
	register BYTE	*to;
	register int	i;

	to = vga_slbuf + pad;
	for (i = 0; i < vt_info.planes; i++) {	/* loop through planes */
		outw(VGA_GRAPH, (READ_MASK) | (vga_read_map[i] << 8));
		memcpy(to, psrc, cnt);
		to += cnt + pad;
	}
}



/* 
 *	vga_slcopyout(pdst, srccnt, dstcnt, startmask, endmask)
 *						-- copy 4 planes of bits from
 *						vga_slbuf buffer to pdst.
 *
 *	Input:
 *		BYTE	*pdst		-- pointer to screen memory
 *		int	srccnt		-- number of bytes in buf per scanline
 *		int	dstcnt		-- number of bytes to copy
 *		BYTE	startmask	-- mask for first byte
 *		BYTE	endmask		-- mask for last byte
 */
vga_slcopyout(pdst, srccnt, dstcnt, startmask, endmask)
BYTE	*pdst;
int	srccnt, dstcnt;
BYTE	startmask, endmask;
{
	register BYTE	*from, *to;
	register int	cnt, inc;
	VOLATILEBYTE t;
	int	i;

	if (--dstcnt == 0) {
		endmask = endmask & startmask;
		startmask = 0;
	}

	inc = srccnt - dstcnt;
	from = vga_slbuf;

	for (i = 0; i < vt_info.planes; i++) {	/* loop through planes */
		to = pdst;
		cnt = dstcnt;
		outw(VGA_SEQ, MAP_MASK | (vga_write_map[i] << 8));

		if (startmask)  {			/* first byte */
			outw(VGA_GRAPH, BITMASK | (startmask << 8));
			t = *to;
			*to++ = *from++;
			cnt--;
		}

		
		outw(VGA_GRAPH, 0xff00 | BITMASK);	/* middle bytes */
		while (--cnt >= 0) {
			*to++ = *from++;
		}

		outw(VGA_GRAPH, BITMASK | (endmask << 8));	/* last byte */
		t = *to;
		*to++ = *from;

		from += inc;
	}
}




/* 
 *	vga_slset(psrc, pdst, dstcnt, startmask, endmask) -- set all active 
 *							planes with the bits in
 *							psrc.
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to bitmap
 *		BYTE	*pdst		-- pointer to screen memory
 *		int	dstcnt		-- number of bytes to copy
 *		BYTE	startmask	-- mask for first byte
 *		BYTE	endmask		-- mask for last byte
 */
vga_slset(psrc, pdst, dstcnt, startmask, endmask)
BYTE	*psrc;
BYTE	*pdst;
int	dstcnt;
BYTE	startmask, endmask;
{
	register BYTE	*to, *from;
	register int	cnt;
	VOLATILEBYTE	t;

	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);	/* all planes on */

	from = psrc;
	to = pdst;
	cnt = dstcnt - 1;

	if (startmask) 
		if (cnt == 0)			/* do only one byte */
			endmask = endmask & startmask;
		else {
			outw(VGA_GRAPH, BITMASK | (startmask << 8));
			t = *to;
			*to++ = *from++;
			cnt--;
		}

	
	outw(VGA_GRAPH, 0xff00 | BITMASK);		/* middle bytes */
	while (--cnt >= 0) {
		/* t = *to; */
		*to++ = *from++;
	}

	outw(VGA_GRAPH, BITMASK | (endmask << 8));	/* last byte */
	t = *to;
	*to = *from;
}



/*
 *	vga_slshift(src, dst, shift, srccnt, invert)	-- shift the data in 
 *							src left putting the 
 *							result in dst.  Invert 
 *							if flag set.
 *
 *	Input:
 *		BYTE	*src		-- source data
 *		BYTE	*dst		-- buffer to store result
 *		int	shift		-- number of bits to shift left by 
 *		int	cnt		-- number of bytes to shift
 *		int	invert		-- true if bytes should be inverted
 */
vga_slshift(src, dst, shift, cnt, invert)
register BYTE	*src, *dst;
int	shift;
register int	cnt;
int	invert;
{
	cnt--;					/* last byte is special */
	/*
	 * Note that this can result in the last byte in one plane 
	 * containing bits from the first byte in the next plane, but
	 * it works out because the last byte in each plane is masked
	 * off when it's written to the display anyway.
	 */
	if (cnt > 0)
	{
	    if (invert) {
		vga_invertRotate (src, dst, cnt, shift);
	    } else {
		vga_rotate (src, dst, cnt, shift);
	    }
	    src += cnt;
	    dst += cnt;
	}

	if (invert)				/* do last byte */
		*dst = ~(*src << shift);
	else
		*dst = *src << shift;
}



/*
 *	vga_slshiftr(src, dst, shift, srccnt, invert)	-- shift the data in 
 *							src right putting the 
 *							result in dst.  Invert 
 *							if flag set.
 *
 *	Input:
 *		BYTE	*src		-- source data
 *		BYTE	*dst		-- buffer to store result
 *		int	shift		-- number of bits to shift right by 
 *		int	cnt		-- number of bytes to shift
 *		int	invert		-- true if bytes should be inverted
 */
vga_slshiftr(src, dst, shift, cnt, invert)
register BYTE	*src, *dst;
int	shift;
register int	cnt;
int	invert;
{
	cnt--;					/* last byte is special */
	/*
	 * Note that this can result in the last byte in one plane 
	 * containing bits from the first byte in the next plane, but
	 * it works out because the last byte in each plane is masked
	 * off when it's written to the display anyway.
	 */
	if (cnt > 0)
	{
	    /* Hack Ho!  vga_invertRotate and vga_rotate actually rotate a
	     * short left by shift bits.  We need to rotate a short right by
	     * shift bits.  This can be done by rotating left by 16-shift
	     * bits.
	     */
	    if (invert) {
		vga_invertRotate (src, dst, cnt, 16-shift);
	    } else {
		vga_rotate (src, dst, cnt, 16-shift);
	    }
	    src += cnt;
	    dst += cnt;
	}

	if (invert)				/* do last byte */
		*dst = ~(*src >> shift);
	else
		*dst = *src >> shift;
}



#ifdef notdef
/*
 *	vga_slsbbmin(psrc, pdst, cnt, pad)	-- copy bytes from psrc to pdst
 *						flipping the bits as we go.
 *
 *	Input:
 *		BYTE	*psrc		-- points to source bitmap sl
 *		BYTE	*pdst		-- points to source bitmap sl
 *		int	cnt		-- number of bytes to copy
 *		int	pad		-- amount to pad before copying
 */		
vga_byteflip(psrc, pdst, cnt, pad)
register BYTE	*psrc;
register BYTE	*pdst;
register int	cnt;
int	pad;
{
	psrc += pad;
	while (--cnt >= 0)
		*pdst++ = vga_bitflip[*psrc++];
}
#endif



/* 
 *	vga_slsbinvertbm(psrc, cnt, startmask, endmask) -- invert all the bits
 *							in the bitmap pointed
 *							to by psrc
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to bitmap
 *		int	cnt		-- number of bytes to copy
 *		BYTE	startmask	-- mask for first byte
 *		BYTE	endmask		-- mask for last byte
 */
vga_slsbinvertbm(psrc, cnt, startmask, endmask)
BYTE	*psrc;
int	cnt;
BYTE	startmask, endmask;
{
	cnt--;

	if (startmask) 
		if (cnt == 0)			/* do only one byte */
			endmask = endmask & startmask;
		else {
			*psrc = (*psrc & ~startmask) | (~*psrc & startmask);
			psrc++;	
			cnt--;
		}

	while (--cnt >= 0) {
		*psrc = ~*psrc;
		psrc++;
	}

	*psrc = (*psrc & ~endmask) | (~*psrc & endmask);
}



/* 
 *	vga_slsbinvert(psrc, cnt)	-- invert all the bytes in the pixmap
 *					pointed to by psrc
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to bitmap
 *		int	cnt		-- number of bytes to copy
 */
vga_slsbinvert(psrc, cnt)
BYTE	*psrc;
int	cnt;
{
	while (--cnt >= 0) {
		*psrc = ~*psrc;
		psrc++;
	}
}



/* 
 *	vga_slsbbltbm(psrc, pdst, cnt, startmask, endmask)
 *					-- copy all the pixels from psrc to pdst
 *					using the current vga_function.
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to source bitmap
 *		BYTE	*pdst		-- pointer to destination bitmap
 *		int	cnt		-- number of bytes to copy
 *		BYTE	startmask	-- mask for first byte
 *		BYTE	endmask		-- mask for last byte
 */
vga_slsbbltbm(psrc, pdst, cnt, startmask, endmask)
register BYTE	*psrc, *pdst;
register int	cnt;
BYTE	startmask, endmask;
{
	cnt--;
	if (startmask) 
		if (cnt == 0)			/* do only one byte */
			endmask = endmask & startmask;
		else {
			switch(vga_function) {
			case VGA_COPY:
				*pdst = (*pdst&~startmask)|(*psrc&startmask);
				break;
			case VGA_OR:
				*pdst |= (*pdst&~startmask)|(*psrc&startmask);
				break;
			case VGA_AND:
				*pdst &= (*pdst&~startmask)|(*psrc&startmask);
				break;
			case VGA_XOR:
				*pdst ^= (*pdst&~startmask)|(*psrc&startmask);
				break;
			}
			psrc++;	
			pdst++;
			cnt--;
		}

	switch(vga_function) {
	case VGA_COPY:
		while (--cnt >= 0)
			*pdst++ = *psrc++;
		break;
	case VGA_OR:
		while (--cnt >= 0)
			*pdst++ |= *psrc++;
		break;
	case VGA_AND:
		while (--cnt >= 0)
			*pdst++ &= *psrc++;
		break;
	case VGA_XOR:
		while (--cnt >= 0)
			*pdst++ ^= *psrc++;
		break;
	}

	switch(vga_function) {
	case VGA_COPY:
		*pdst = (*pdst & ~endmask) | (*psrc & endmask);
		break;
	case VGA_OR:
		*pdst |= (*pdst & ~endmask) | (*psrc & endmask);
		break;
	case VGA_AND:
		*pdst &= (*pdst & ~endmask) | (*psrc & endmask);
		break;
	case VGA_XOR:
		*pdst ^= (*pdst & ~endmask) | (*psrc & endmask);
		break;
	}
}



/* 
 *	vga_sbset(psrc, pdst, cnt, startmask, endmask)
 *					-- copy all the pixels from psrc to pdst
 *					using the current vga_function.  Pdst
 *					is in system memory, not VGA memory.
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to source bitmap
 *		BYTE	*pdst		-- pointer to destination bitmap
 *		int	cnt		-- number of bytes to copy
 *		BYTE	startmask	-- mask for first byte
 *		BYTE	endmask		-- mask for last byte
 */
vga_sbset(psrc, pdst, cnt, startmask, endmask)
register BYTE	*psrc, *pdst;
register int	cnt;
BYTE	startmask, endmask;
{
	cnt--;
	if (startmask) 
		if (cnt == 0)			/* do only one byte */
			endmask = endmask & startmask;
		else {
			*pdst = (*pdst & ~startmask) | (*psrc & startmask);
			psrc++;	
			pdst++;
			cnt--;
		}

	while (--cnt >= 0)
		*pdst++ = *psrc++;

	*pdst = (*pdst & ~endmask) | (*psrc & endmask);
}
