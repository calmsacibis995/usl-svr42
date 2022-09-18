/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256bstpl.c	1.2"

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
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"

#define MAX_STIPPLE_WIDTH	256


/*
 *	v256_big_stpl_setup()	-- Set up a stipple for filling.  Expand
 *				the stipple if it's width is a multiple
 *				of 8 (since it's easy, and can speed
 *				things up quite a bit.)
 */
SIBool
v256_big_stpl_setup()
{
	SIbitmapP 	bmap;
	register BYTE		*src, *dst, *pdst;
	int		srcinc, dstinc, i, j, expand, bytewidth;
	extern		BYTE *malloc();

	bmap = &(v256_gs->raw_stipple);

	/*
	 * if the pattern is already large, or it's not a multiple of
	 * 8 wide, don't do anything.
	 */
	if ((bmap->Bwidth >= MAX_STIPPLE_WIDTH) || (bmap->Bwidth & 0x7))
		return(SI_SUCCEED);

	/*
	 * expand the pattern to a width >= MAX_STIPPLE_WIDTH
	 */
	expand = (MAX_STIPPLE_WIDTH + bmap->Bwidth) / bmap->Bwidth;
	srcinc = ((bmap->Bwidth + 31) & ~31) >> 3;
	dstinc = (((expand * bmap->Bwidth) + 31) & ~31) >> 3;
	src = v256_gs->big_stpl;
	dst = malloc(bmap->Bheight * dstinc);
	if (!dst)
		return(SI_FAIL);
	
	bytewidth = bmap->Bwidth >> 3;
	for (i = 0; i < bmap->Bheight; i++) {
		pdst = dst + (dstinc * i);
		for (j = 0; j < expand; j++) {
			bcopy(src, pdst, bytewidth);
			pdst += bytewidth;
		}
		src += srcinc;
	}

	free(v256_gs->big_stpl);
	v256_gs->big_stpl = dst;
	bmap->Bwidth *= expand;
	return(SI_SUCCEED);
}


/*
 * 	v256_big_stpl_rect(prect) -- fill a rectangle with a large stipple.
 *
 *	Input:
 *		SIRectP prect	  -- pointer to rectangle to fill	
 */
SIBool
v256_big_stpl_rect(prect)
SIRectP prect;
{
	int sh, sy, h, ycnt, xcnt;
	int x1, y1, x2, y2;
	extern void v256_big_hline_stpl();

	x1 = prect->ul.x;
	y1 = prect->ul.y;
	x2 = prect->lr.x - 1;
	y2 = prect->lr.y - 1;

	if ((x1 > x2) || (y1 > y2))
		return(SI_SUCCEED);

	/*
	 * Clip points
	 */
	if ((x1 > v256_clip_x2) || (x2 < v256_clip_x1) ||
	    (y1 > v256_clip_y2) || (y2 < v256_clip_y1))
		return(SI_SUCCEED);

	if (x1 < v256_clip_x1) x1 = v256_clip_x1;
	if (x2 > v256_clip_x2) x2 = v256_clip_x2;
	if (y1 < v256_clip_y1) y1 = v256_clip_y1;
	if (y2 > v256_clip_y2) y2 = v256_clip_y2;

	/*
	 * BUGFIX : 
	 *	we are missing one pixel in height and width by  doing
	 *	ycnt = y2 - y1;
	 *	xcnt = x2 - x1;
	 */
	ycnt = y2 - y1 + 1;
	xcnt = x2 - x1 + 1;
	sh = v256_gs->raw_stipple.Bheight;
	sy = (y1 - v256_gs->raw_stipple.BorgY) % sh;
	if (sy < 0) sy += sh;

	/*
	 * do lines that lead up to the full stipple
	 */
	h = sh - sy;
	if (h > ycnt)
		h = ycnt;

	v256_big_hline_stpl(x1, y1, xcnt, h);
	ycnt -= h;
	y1 += h;

	/*
	 * do the middle (full) stipples
	 */
	while (ycnt > sh) {
		v256_big_hline_stpl(x1, y1, xcnt, sh);
		ycnt -= sh;
		y1 += sh;
	}

	/*
	 * do final lines
	 */
	if (ycnt)
		v256_big_hline_stpl(x1, y1, xcnt, ycnt);
	return(SI_SUCCEED);
}



/*
 *	v256_big_hline_stpl(x, y, xcnt, ycnt)	-- draw xcnt pixels 
 *				horizontally starting at (x, y) using the
 *				current stipple pattern.  This works by
 *				blitting the scanlines from the downloaded 
 *				stipple onto the line being filled using
 *				the v256_ms_stplblt() routine.  For ycnt > 1, 
 *				blit multiple lines.  This does no check
 *				to make sure there are ycnt lines available 
 *				to be blitted.  Caller beware.
 *
 *	Input:
 *		int		x	-- x position of starting point
 *		int		y	-- y position of starting point
 *		int		xcnt	-- number of pixels to draw
 *		int		ycnt	-- number of lines to draw
 */
void
v256_big_hline_stpl(x, y, xcnt, ycnt)
int		x, y;
register int	xcnt;
int		ycnt;
{
	register int	sw;
	int		sh, sx, sy;
	int		w;
	extern int	v256_ms_stplblt();
	
	sw = v256_gs->raw_stipple.Bwidth;
	sh = v256_gs->raw_stipple.Bheight;
	sx = (x - v256_gs->raw_stipple.BorgX) % sw;
	sy = (y - v256_gs->raw_stipple.BorgY) % sh;
	if (sx < 0) sx += sw;
	if (sy < 0) sy += sh;

	/* 
	 * Do leading pixels up to the start of the stipple
	 */
	w = sw - sx;				/* starting blit width */
	if (w > xcnt)
		w = xcnt;

	v256_ms_stplblt(&(v256_gs->raw_stipple), sx, sy, x, y, w, ycnt, 0, 0);

	xcnt -= w;
	if (!xcnt)				/* narrow stipple fill */
		return;
	x += w;

	/*
	 * Do the middle (full) blits
	 */
	while (xcnt > sw) {
		v256_ms_stplblt(&(v256_gs->raw_stipple), 0, sy,x,y,sw,ycnt,0,0);
		xcnt -= sw;
		x += sw;
	}

	/*
	 * Do the last (partial) blit
	 */
	v256_ms_stplblt(&(v256_gs->raw_stipple), 0, sy, x, y, xcnt, ycnt, 0,0);
}
