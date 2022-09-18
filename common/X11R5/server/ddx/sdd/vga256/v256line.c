/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256line.c	1.1"

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

extern unsigned long v256_expand[];

/*
 *	v256_line_onebit(cnt, ptsIn)	-- draw a series of lines based on
 *					points passed in.  Lines are 1 pixel
 *					wide and are painted with the current
 *					line style, foreground and background
 *					colors, and current ROP.
 *
 *	Input:
 *		int		cnt	-- count of number of points
 *		DDXPointRec	*ptsIn	-- Points
 */
SIBool
v256_line_onebit(cnt, ptsIn)
register int		cnt;
register DDXPointRec	*ptsIn;
{
	DDXPointRec     tpt;
	register int    firsttime = 1;

	DBENTRY("v256_line_onebit()");

	if (v256_gs->mode == GXnoop)
		return(SI_SUCCEED);

	if (--cnt <= 0)
		return(SI_SUCCEED);
	v256_draw_line(*(ptsIn), *(ptsIn+1));
	ptsIn++;

	while (--cnt) {
		/*
		 * To correctly draw the poly line we must only draw each
		 * pixel once. So in each line segment, except for the first,
		 * we must skip the first pixel and start with the next pixel
		 * in the line.
		 */
		v256_calc_stpt(*ptsIn, *(ptsIn+1), &tpt);
		v256_draw_line(tpt, *(ptsIn+1));
		ptsIn++;
	}
	return(SI_SUCCEED);
}


/*
 *	v256_lineseg_onebit(cnt, ptsIn)	-- draw a series of lines based on
 *					points passed in.  Lines are 1 pixel
 *					wide and are painted with the current
 *					line style, foreground and background
 *					colors, and current ROP.
 *
 *	Input:
 *		int		cnt	-- count of number of points (2*lines)
 *		DDXPointRec	*ptsIn	-- Points
 */
SIBool
v256_lineseg_onebit(cnt, ptsIn)
register int		cnt;
register DDXPointRec	*ptsIn;
{

	DBENTRY("v256_lineseg_onebit()");

	if (v256_gs->mode == GXnoop)
		return(SI_SUCCEED);
	if (cnt <= 0)
		return(SI_SUCCEED);
	cnt >>= 1;

	while (cnt) {
		v256_draw_line(*(ptsIn), *(ptsIn+1));
		ptsIn += 2;
		cnt--;
	}
	return(SI_SUCCEED);
}


/*
 *      v256_calc_stpt(pt1, pt2)     -- calculate the correct starting point to
 *                                      continue the polyline segment drawing.
 *
 *      Input:
 *              DDXPointRec     pt1  -- first point in line
 *              DDXPointRec     pt2  -- last point in line
 *              DDXPointRec     *pt3 -- return value
 *                                      the correct first point
 */
v256_calc_stpt(pt1, pt2, pt3)
DDXPointRec     pt1;
DDXPointRec     pt2;
DDXPointRec     *pt3;
{
    int x,y;            /* current point on the line */
    int dx, dy, adx, ady;
    int signdx, signdy, len;
    int du, dv;
    int e, e1, e2;

    dx = pt2.x - pt1.x;
    dy = pt2.y - pt1.y;
    adx = abs(dx);
    ady = abs(dy);
    signdx = sign(dx);
    signdy = sign(dy);

    x = pt1.x;
    y = pt1.y;
    if (adx > ady) {
	/* X_AXIS */
	e = (ady << 1) - adx;
	if (adx > 1) {
	    if (((signdx > 0) && (e < 0)) ||
		((signdx <=0) && (e <=0))
	       ) {
		x+= signdx;
	    } else {
		/* initialize next span */
		x += signdx;
		y += signdy;
	    }
	}
    }
    else
    {
	/* Y_AXIS */
	e = (adx << 1) - ady;
	if (ady > 1) {
	    if (((signdx > 0) && (e < 0)) ||
		((signdx <=0) && (e <=0))
	       ) {
		;
	    } else {
		x += signdx;
	    }
	    y += signdy;
	}
    }

    /* (x,y) is the next pixel location */
    pt3->x = x;
    pt3->y = y;
}



v256_line_horiz(pt1, pt2, count)
DDXPointRec	pt1;
DDXPointRec	pt2;
int count;
{
	unsigned short int left, right, last;
	unsigned int y1;
	int	     dst;

	left = pt1.x;
	right = pt2.x;

	y1 = pt1.y * v256_slbytes;

	dst = y1 + left;
	y1 &= VIDEO_PAGE_MASK;
	selectpage(dst);
	left += y1;
	right += y1;

	while (--count >= 0) {
		if (left > right) {
			filler(v256_fb+left, v256_fb+VIDEO_PAGE_MASK);
			selectpage((dst & ~VIDEO_PAGE_MASK) + (VIDEO_PAGE_MASK+1));
			filler(v256_fb, v256_fb+right);
		}
		else
			filler(v256_fb+left, v256_fb+right);

		last = right;
		left += v256_slbytes;
		right += v256_slbytes;
		dst += v256_slbytes;

		if (last > left)
			selectpage(dst);
	}
}



/*
 *	v256_draw_line(pt1, pt2)	-- draw a one bit line from pt1 to pt2
 *					using the current linestyle.
 *
 *	Input:
 *		DDXPointRec	pt1		-- first point in line
 *		DDXPointRec	pt2		-- last point in line
 */
v256_draw_line(pt1, pt2)
DDXPointRec	pt1;
DDXPointRec	pt2;
{
	register BYTE *bufp, mask, color;
	int i1, i2, d1, d2, d;
	unsigned short left, right;
	int dst;
	register int y1;
	int y2;
	DDXPointRec	tmp;

	if (!v256_clip_line(&pt1, &pt2))
		return;				/* out of bounds */

	/*
	 * Breshenham algorithm to draw lines a 256-color byte-packed 
	 * display buffer.
	 */

	d2 = pt2.y - pt1.y;
	d1 = pt2.x - pt1.x;

	if(d2 == 0) {			/* horizontal line */
		if (pt1.x < pt2.x)
			v256_line_horiz(pt1, pt2, 1);
		else
			v256_line_horiz(pt2, pt1, 1);
		return;
	}

	/*
	 * for long shallow lines, we do some special things
	 */
	if ((d1 > 8 || d1 < -8) &&  ((d2 == 1) || (d2 == -1))) {
		tmp.x = (pt1.x + pt2.x)/2;
		if (pt1.x < pt2.x) {
			tmp.y = pt1.y;
			v256_line_horiz(pt1, tmp, 1);
			tmp.x++;
			tmp.y += d2;
			v256_line_horiz(tmp, pt2, 1);
		}
		else {
			tmp.y = pt2.y;
			v256_line_horiz(pt2, tmp, 1);
			tmp.x++;
			tmp.y -= d2;
			v256_line_horiz(tmp, pt1, 1);
		}
		return;
	}

	y1 = pt1.y * v256_slbytes;
	y2 = pt2.y * v256_slbytes;

	dst = y1 + pt1.x;
	selectpage(dst);

	bufp = v256_fb + ((pt1.x + y1) & VIDEO_PAGE_MASK);
	color = v256_src;
	mask = v256_gs->pmask;

	i1 = 1;
	if (d1 < 0) {
		d1 = -d1;
		i1 = -1;
	}

	i2 = v256_slbytes;
	if (d2 < 0) {
		d2 = -d2;
		i2 = -v256_slbytes;
	}

	switch(v256_function) {
	case V256_COPY:
		mask = ~mask;
		break;

	case V256_AND:
	case V256_AND_INVERT:
		color |= ~mask;
		break;
	}


	if(d1 > d2) {
		d = d2 + d2 - d1;
		d1 += d1;
		d2 += d2;
		while(1) {
			switch(v256_function) {
			case V256_COPY:
				if (mask == 0)
					*bufp = color;
				else
					*bufp = (*bufp & mask) | color;
				break;
			case V256_AND:
				*bufp &= color;
				break;
			case V256_OR:
				*bufp |= color;
				break;
			case V256_XOR:
				*bufp ^= color;
				break;
			case V256_AND_INVERT:
				*bufp = (*bufp & color) ^ mask;
				break;
			case V256_OR_INVERT:
				*bufp = (*bufp | color) ^ mask;
				break;
			}

			if(pt1.x == pt2.x)
				break;

			if(d >= 0) {
				d -= d1;
				y1 += i2;
				bufp += i2;
			}

			d += d2;
			pt1.x += i1;
			bufp += i1;
			if (((pt1.x + y1) ^ dst) & ~VIDEO_PAGE_MASK) {
				bufp = v256_fb + ((pt1.x+y1) & VIDEO_PAGE_MASK);
				dst = pt1.x + y1;
				selectpage(dst);
			}
		}
	}
	else {
		d = d1 + d1 - d2;
		d1 += d1;
		d2 += d2;
		while (1) {
			switch(v256_function) {
			case V256_COPY:
				if (mask == 0)
					*bufp = color;
				else
					*bufp = (*bufp & mask) | color;
				break;
			case V256_AND:
				*bufp &= color;
				break;
			case V256_OR:
				*bufp |= color;
				break;
			case V256_XOR:
				*bufp ^= color;
				break;
			case V256_AND_INVERT:
				*bufp = (*bufp & color) ^ mask;
				break;
			case V256_OR_INVERT:
				*bufp = (*bufp | color) ^ mask;
				break;
			}

			if(y1 == y2)
				break;
			if(d >= 0) {
				d -= d2;
				pt1.x += i1;
				bufp += i1;
			}
			d += d1;
			y1 += i2;
			bufp += i2;
			if (((pt1.x + y1) ^ dst) & ~VIDEO_PAGE_MASK) {
				bufp = v256_fb + ((pt1.x+y1) & VIDEO_PAGE_MASK);
				dst = pt1.x + y1;
				selectpage(dst);
			}
		}
	}
}



filler(left, right)
register BYTE *left, *right;
{
	register unsigned long color;
	unsigned long mask, keepmask;

	mask = v256_expand[v256_gs->pmask];
	color = v256_expand[v256_src & mask];
	keepmask = ~mask;

	switch (v256_function) {
	case V256_COPY:
		if (keepmask == 0) {
			v256_memset(left, color, (int)(right - left + 1));
			return;
		}
		v256_memsetmask(left, color, (int)(right - left + 1), keepmask);
		return;

	case V256_XOR:
		do {
			*left++ ^= color;
		} while (left <= right);
		return;

	case V256_AND:
		color |= keepmask;
		do {
			*left++ &= color;
		} while (left <= right);
		return;

	case V256_OR:
		do {
			*left++ |= color;
		} while (left <= right);
		return;

	case V256_AND_INVERT:
		color |= keepmask;
		do {
			*left = (*left & color) ^ mask;
			left++;
		} while (left <= right);
		return;

	case V256_OR_INVERT:
		do {
			*left = (*left | color) ^ mask;
			left++;
		} while (left <= right);
		return;
	}
}
