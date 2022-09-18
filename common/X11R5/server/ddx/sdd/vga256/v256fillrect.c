/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256fillrect.c	1.2"

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

/*
 *	FILE : v256fillrct.c
 *	DESCRIPTION : This file implements filled rectangles
 *	Filled rectangles :
 */

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "v256bitblt.h"

/*
 * get the cfb macros
 */
#define	PPW		4
#define	PMSK	0xFF
#include "cfbmskbits.h"
#include "cfb8bit.h"

#define	NDEBUG
#include	<assert.h>
/*
 *	v256_rotate_stipple
 */

/*
 *	v256_rotate_expand_stipple(src, dst, w, h, x, y)	-- Given a bitmap in src, 
 *						pad it to exactly 32 bits
 *						of width, align it at 0, 0
 *						and store it in dst.  
 *
 *	Input:
 *		BYTE	*src	-- source bitmap
 *		BYTE	*dst	-- destination
 *		int	w	-- width of source bitmap
 *		int	h	-- height of source bitmap
 *		int	x	-- x origin (in bits) of source bitmap
 *		int	y	-- y origin (in bits) of source bitmap
 *
 *	Returns:
 *		1 for normal patterns
 *		0 if the pattern has a bad width
 */
v256_rotate_expand_stipple(src, dst, w, h, x, y)
BYTE	*src, *dst;
int	w, h, x, y;
{
	BITS16 *pat_word, mask;
	BITS32 *pat_long, *dst_word;
	int	i, hcnt;
	int	shift;

	/*
	 *  Repeat the pattern (if it's width is 1, 2, 4, or 8 bits) to fill
	 *  a 32-bit word.
	 */	
	if ( w == 32 )
	{
		return	SI_TRUE;
	}
	pat_word = (BITS16 *)src;
	for (i = 0; i < h; i++) 
	{
		switch (w) {
		case 16:
			break;
		case 8:
			*pat_word &= 0xff;
			*pat_word |= *pat_word << 8;
			break;

		case 1:
			*pat_word &= 0x1;
			*pat_word |= *pat_word << 1;
		case 2:
			*pat_word &= 0x3;
			*pat_word |= *pat_word << 2;
		case 4:
			*pat_word &= 0xf;
			*pat_word |= *pat_word << 4;
			*pat_word |= *pat_word << 8;
			break;
		default:
			return (SI_FALSE);		/* bad pattern */
		}
		*(pat_word+1) = *pat_word;
		pat_word += 2;
	}

	/*
	 * Shift the pattern as needed and store the result in the dst.
	 * Note that we also may need to start in the middle if the
	 * pattern is mis-aligned on the y axis.
	 */
	y %= h;
	hcnt = (y? (h - y) : 0);

	pat_long = ((BITS32 *)src) + hcnt;
	dst_word = (BITS32 *)dst;

	shift = 32 - (x & 0x1F);
	

	for (i = 0; i < h; i++, hcnt++) 
	{
		if (hcnt == h) 
		{
			hcnt = 0;
			pat_long = (BITS32 *)src;
		}
		*dst_word++ = (BITS32)RotBitsLeft(*pat_long, shift);
		pat_long ++;
	}
	return(1);
}

#ifndef	DEBUG
/*
 *	v256_fast_fillrect(cnt, prect)	-- draw a series of filled rectangles.
 *				The current fill style, foreground and 
 *				background colors, and current ROP are used.
 *
 *	Input:
 *		int	cnt		-- number of rectangles to fill
 *		SIRectP	prect		-- pointer to list of rectangles
 */

SIBool
v256_fast_fillrect(nRects, pRect)
register int	 	nRects;
register SIRectP	pRect;
{
		
		/*
		 *	standard check
		 */
		if ( v256_gs->mode == GXnoop )
		{
			return (SI_SUCCEED);
		}

		/*
		 *	Check if opaque stippling is needed : else call the standard routine
		 */
		if ( v256_gs->fill_mode != SGFillStipple 
				|| v256_gs->stp_mode != SGOPQStipple )
		{
			v256_fill_rect(nRects, pRect);
		}
		else
		{
			/*
			 * setup the reduced rop lookup etc
			 */
			cfb8CheckOpaqueStipple(v256_gs->mode,
				v256_gs->fg, v256_gs->bg,v256_gs->pmask);
			/*
			 *	Go to our routine
			 */
			v256_fast_stipplefillrect(nRects, pRect);
		}
}

/*
 *	v256_fast_fillrect : draw a series of stippled filled rectangles
 *	The stipple pattern is assumed already rotated and present.
 *	This routine will clip and call v256_ms_stplblt appropriately
 */
static 
v256_fast_stipplefillrect(register int nrects, register SIRectP pRect)
{
	int	count;
	/*
	 *	Coordinates (inclusive) of box to be stippled on screen
	 */
	int	ulx, uly, lrx, lry;	

	for (count = 0;count < nrects; count++, pRect++)
	{
		/*
		 * 	the split rectangles, the number of 
		 * 	such split rectangles , and a temporary
		 */
		VgaRegion	subRectangles[MAX_VGA_PAGES *3];
		int		num_subrects,i;

		/*
		 *	Clip points : SIRectP definition says that lr point is excluded
		 */
		if ( (pRect->ul.x > v256_clip_x2) ||
			(pRect->lr.x - 1 < v256_clip_x1) ||
			(pRect->ul.y > v256_clip_y2) ||
			(pRect->lr.y - 1 < v256_clip_y1) )
		{
			continue;
		}
		if ( pRect->ul.x >= pRect->lr.x || pRect->ul.y >= pRect->lr.y )
		{
			continue;
		}
		
		/*
		 *	Get the four inclusive corners
		 */
		ulx = ( pRect->ul.x < v256_clip_x1 ) ? v256_clip_x1
						: pRect->ul.x;
		
		uly = (pRect->ul.y < v256_clip_y1 ) ? v256_clip_y1
						: pRect->ul.y;
		
		lrx = (pRect->lr.x - 1 > v256_clip_x2) ? v256_clip_x2 
						: pRect->lr.x - 1;
		
		lry = (pRect->lr.y - 1 > v256_clip_y2 )? v256_clip_y2
						: pRect->lr.y - 1;
		
		/*
		 * split the destination into managable rectangles
		 */
		v256_split_request(ulx,uly,lrx,lry,&num_subrects,
					subRectangles);
		/* 
		 * call the fast stippling routine
		 */
		for (i = 0; i < num_subrects; i++)
		{
			/*
			 * select the appropriate page
			 */
			selectpage(
				OFFSET(subRectangles[i].x,subRectangles[i].y));
	
			v256_vgapage_opaque_stipple(&subRectangles[i]);

		}

	}
}

#endif	/* !! debug */
#ifdef	DEBUG
/*
 *	Testing pat_setup
 */

int
main(int  argc, char **argv)
{
	int	test_pat[] = { 0x0F, 0xF0, 0x03, 0x30, 0x0F, 0xF0, 0x03, 0x30};
	int		dest[10];

	v256_rotate_expand_stipple(&test_pat, dest, 8, 8, 3, 3);
}

#endif
