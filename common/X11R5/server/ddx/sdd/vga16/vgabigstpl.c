/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgabigstpl.c	1.3"

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

#define MAX_STIPPLE_WIDTH	256


/*
 *	vga_big_stpl_setup()	-- Set up a stipple for filling.  Expand
 *				the stipple if it's width is a multiple
 *				of 8 (since it's easy, and can speed
 *				things up quite a bit.)
 */
SIBool
vga_big_stpl_setup()
{
	SIbitmapP 	bmap;
	register BYTE		*src, *dst, *pdst;
	int		srcinc, dstinc, i, j, expand, bytewidth;
	extern		BYTE *malloc();

	bmap = &(vga_gs->raw_stipple);

	/*
	 * if the pattern is already large, don't do anything.
	 */
	if (bmap->Bwidth >= MAX_STIPPLE_WIDTH)
		return(SI_SUCCEED);

	/*
	 * expand the pattern to a width >= MAX_STIPPLE_WIDTH
	 */
	expand = (MAX_STIPPLE_WIDTH + bmap->Bwidth) / bmap->Bwidth;
	srcinc = ((bmap->Bwidth + 31) & ~31) >> 3;
	dstinc = (((expand * bmap->Bwidth) + 31) & ~31) >> 3;
	src = vga_gs->big_stpl;
	dst = malloc(bmap->Bheight * dstinc);
	if (!dst)
		return(SI_FAIL);

	/* If the stipple is a multiple of 8 wide, we can expand it easily
	 * with bcopy.  Otherwise, ugly shifting is involved.
	 */
	bytewidth = (bmap->Bwidth + 7) >> 3;
	if (bmap->Bwidth & 0x7)
	{
	    int		shift;
	    int		shiftcnt;
	    int		dx;
	    BYTE	*psrc;

	    for (i = 0; i < expand; i++)
	    {
		dx = bmap->Bwidth * i;
		shift = dx & 0x7;
		shiftcnt = (bmap->Bwidth + shift + 0x7) >> 3;
		pdst = dst + (dx >> 3);
		psrc = src;
		for (j = 0; j < bmap->Bheight; j++)
		{
		    vga_shiftl (psrc, pdst, shiftcnt, shift);
		    psrc += srcinc;
		    pdst += dstinc;
		}
	    }
	}
	else
	{
	    for (i = 0; i < bmap->Bheight; i++) {
		pdst = dst + (dstinc * i);
		for (j = 0; j < expand; j++) {
		    bcopy(src, pdst, bytewidth);
		    pdst += bytewidth;
		}
		src += srcinc;
	    }
	}

	bmap->Bwidth *= expand;
	free(vga_gs->big_stpl);
	vga_gs->big_stpl = dst;
	return(SI_SUCCEED);
}


/*
 * 	vga_big_stpl_rect(prect) -- fill a rectangle with a large stipple.
 *
 *	Input:
 *		SIRectP	prect		-- pointer to rectangle to fill
 */
SIBool
vga_big_stpl_rect(prect)
SIRectP prect;
{
	int sh, sy, h, ycnt, xcnt;
	int x1, y1, x2, y2;

	x1 = prect->ul.x;
	y1 = prect->ul.y;
	x2 = prect->lr.x - 1;
	y2 = prect->lr.y - 1;

	if ((x1 > x2) || (y1 > y2))
		return(SI_SUCCEED);

	/*
	 * Clip points
	 */
	if ((x1 > vga_clip_x2) || (x2 < vga_clip_x1) ||
	    (y1 > vga_clip_y2) || (y2 < vga_clip_y1)) {
		return(SI_SUCCEED);
	}

	if (x1 < vga_clip_x1) x1 = vga_clip_x1;
	if (x2 > vga_clip_x2) x2 = vga_clip_x2;
	if (y1 < vga_clip_y1) y1 = vga_clip_y1;
	if (y2 > vga_clip_y2) y2 = vga_clip_y2;

	ycnt = y2 - y1 + 1;
	xcnt = x2 - x1 + 1;
	sh = vga_gs->raw_stipple.Bheight;
	sy = (y1 - vga_gs->raw_stipple.BorgY) % sh;
	if (sy < 0) sy += sh;

	/*
	 * do lines that lead up to the full stipple
	 */
	h = sh - sy;
	if (h > ycnt)
		h = ycnt;

	vga_big_hline_stpl(x1, y1, xcnt, h);
	ycnt -= h;
	y1 += h;

	/*
	 * do the middle (full) stipples
	 */
	while (ycnt > sh) {
		vga_big_hline_stpl(x1, y1, xcnt, sh);
		ycnt -= sh;
		y1 += sh;
	}

	/*
	 * do final lines
	 */
	if (ycnt)
		vga_big_hline_stpl(x1, y1, xcnt, ycnt);
}



/*
 *	vga_big_hline_stpl(x, y, xcnt, ycnt)	-- draw xcnt pixels 
 *				horizontally starting at (x, y) using the
 *				current stipple pattern.  This works by
 *				blitting the scanlines from the downloaded 
 *				stipple onto the line being filled using
 *				the vga_ms_stplblt() routine.  For ycnt > 1, 
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
vga_big_hline_stpl(x, y, xcnt, ycnt)
int		x, y;
register int	xcnt;
int		ycnt;
{
	register int	sw;
	int		sh, sx, sy;
	int		w;
	
	sw = vga_gs->raw_stipple.Bwidth;
	sh = vga_gs->raw_stipple.Bheight;
	sx = (x - vga_gs->raw_stipple.BorgX) % sw;
	sy = (y - vga_gs->raw_stipple.BorgY) % sh;
	if (sx < 0) sx += sw;
	if (sy < 0) sy += sh;

	/* 
	 * Do leading pixels up to the start of the stipple
	 */
	w = sw - sx;				/* starting blit width */
	if (w > xcnt)
		w = xcnt;

	vga_ms_stplblt(&(vga_gs->raw_stipple), sx, sy, x, y, w, ycnt, 0, 0);
	xcnt -= w;
	if (!xcnt)				/* narrow stipple fill */
		return;
	x += w;

	/*
	 * Do the middle (full) blits
	 */
	while (xcnt > sw) {
		vga_ms_stplblt(&(vga_gs->raw_stipple), 0, sy, x, y,sw,ycnt,0,0);
		xcnt -= sw;
		x += sw;
	}

	/*
	 * Do the last (partial) blit
	 */
	vga_ms_stplblt(&(vga_gs->raw_stipple), 0, sy, x, y, xcnt, ycnt, 0, 0);
}
