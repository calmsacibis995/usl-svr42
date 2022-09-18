/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgaline.c	1.3"

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
#include "vga.h"
#include "sys/inline.h"



/*
 *	vga_line_onebit(cnt, ptsIn)	-- draw a series of lines based on
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
vga_line_onebit(cnt, ptsIn)
register int		cnt;
register DDXPointRec	*ptsIn;
{
	DDXPointRec     tpt;
	register int    firsttime = 1;

	DBENTRY("vga_line_onebit()");

	if (vga_gs->mode == GXnoop)
		return(SI_SUCCEED);

	outw(VGA_GRAPH, GR_ENAB_SR | vt_allplanes);	/* enable SR reg */
	outw(VGA_GRAPH, GR_SR | vga_color_map[vga_src] << 8);	/* set color */
	outw(VGA_GRAPH, GR_FUNC | vga_function);	/* set rop */

	for (;--cnt; ptsIn++) {

		/*
		 * To correctly draw the poly line we must only draw each
		 * pixel once. So in each line segment, except for the first,
		 * we must skip the first pixel and start with the next pixel
		 * in the line.
		 */
		if(firsttime) {
		    firsttime = 0;
		    tpt.x = ptsIn[0].x;
		    tpt.y = ptsIn[0].y;
		}
		else
		    vga_calc_stpt(*ptsIn, *(ptsIn+1), &tpt);

		if (vga_invertdest == SI_TRUE) {
			outw(VGA_GRAPH, GR_FUNC | VGA_XOR);
			outw(VGA_GRAPH, GR_SR | vga_gs->pmask); 

			vga_draw_line(tpt, *(ptsIn+1));

			if (vga_gs->mode == GXinvert)
				continue;

			outw(VGA_GRAPH, GR_SR | vga_color_map[vga_src]<<8);
			outw(VGA_GRAPH, GR_FUNC | vga_function);
		}

		vga_draw_line(tpt, *(ptsIn+1));
	}

	outw(VGA_GRAPH, GR_ENAB_SR);			/* disable SR reg */
	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* restore rop */

	return(SI_SUCCEED);
}

/*
 *      vga_calc_stpt(pt1, pt2)       -- calculate the correct starting point to
 *                                       continue the polyline segment drawing.
 *
 *      Input:
 *              DDXPointRec     pt1             -- first point in line
 *              DDXPointRec     pt2             -- last point in line
 *              DDXPointRec     *pt3            -- return value
 *                                                 the correct first point
 */
vga_calc_stpt(pt1, pt2, pt3)
DDXPointRec     pt1;
DDXPointRec     pt2;
DDXPointRec     *pt3;
{
    int x,y;            /* current point on the line */
    int dx, dy, adx, ady;
    int signdx, signdy;
    int e;

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

/*
 *	vga_draw_line(pt1, pt2)		-- draw a one bit line from pt1 to pt2
 *					using the current linestyle.
 *
 *	Input:
 *		DDXPointRec	pt1		-- first point in line
 *		DDXPointRec	pt2		-- last point in line
 */
vga_draw_line(pt1, pt2)
DDXPointRec	pt1;
DDXPointRec	pt2;
{
	DDXPointRec	tpt;

	if (!vga_clip_line(&pt1, &pt2))
		return;				/* out of bounds */

	if (pt2.x < pt1.x) 			/* swap points */
		vga_swap(pt1, pt2, tpt);

	if (pt1.y == pt2.y)			/* horizontal line */
		vga_line_horiz(pt1, pt2, 1);
	else {
		if (pt1.x == pt2.x)		/* vertical line */
			if (pt1.y < pt2.y)
				vga_line_horiz(pt1, pt2, (pt2.y - pt1.y) + 1);
			else
				vga_line_horiz(pt2, pt1, (pt1.y - pt2.y) + 1);
		else				/* sloped line */
			vga_line_bres(pt1, pt2);
	}
}



/*
 *	vga_line_horiz(pt1, pt2, ycnt)	-- draw ycnt horizontal lines from
 *					pt1 to pt2 using the current linestyle.
 *					Pt1 is guarenteed to be on the "left".
 *					This is also used for vertical lines!
 *
 *	Input:
 *		DDXPointRec	pt1	-- first point in line
 *		DDXPointRec	pt2	-- last point in line
 *		int		ycntx	-- number of lines to draw
 */
vga_line_horiz(pt1, pt2, ycnt)
DDXPointRec	pt1;
DDXPointRec	pt2;
int	ycnt;
{
	register BYTE startmask, endmask;
	register int xcnt;
	BYTE *paddr;

	paddr     = vga_fb + vga_byteoffset(pt1.x, pt1.y);
	startmask = vga_start_bits[pt1.x & 0x7];
	endmask   = vga_end_bits[pt2.x & 0x7];
	xcnt 	  = (pt2.x >> 0x3) - (pt1.x >> 0x3);

	outb(VGA_GRAPH, BITMASK);
	if (startmask != 0xff)	 		/* do the first byte */
		if (xcnt == 0)			/* only one byte to do */
			endmask &= startmask;
		else {
			xcnt--;
			outb(VGA_GRAPH+1, startmask);
			vga_set1(paddr++, ycnt);
		}
			
	if (endmask == 0xff)
		xcnt++;
	else {
		outb(VGA_GRAPH+1, endmask);	/* do final byte */
		vga_set1(paddr+xcnt, ycnt);
	}

	if (xcnt) {
		outb(VGA_GRAPH+1, 0xff);	/* do middle bytes */
		if (vga_gs->mode == GXcopy)
			vga_solidn(paddr, xcnt, ycnt);
		else
			vga_setn(paddr, xcnt, ycnt);
	}
}



/*
 *	vga_line_horiz1(pt1, pt2)	-- draw one horizontal line from
 *					pt1 to pt2 using the current linestyle.
 *					Pt1 is guarenteed to be on the "left".
 *
 *	Input:
 *		DDXPointRec	pt1	-- first point in line
 *		DDXPointRec	pt2	-- last point in line
 */
vga_line_horiz1(pt1, pt2)
DDXPointRec	pt1;
DDXPointRec	pt2;
{
	register BYTE startmask, endmask;
	int xcnt;
	register BYTE *paddr;

	paddr     = vga_fb + vga_byteoffset(pt1.x, pt1.y);
	startmask = vga_start_bits[pt1.x & 0x7];
	endmask   = vga_end_bits[pt2.x & 0x7];
	xcnt 	  = (pt2.x >> 0x3) - (pt1.x >> 0x3);

	outb(VGA_GRAPH, BITMASK);
	if (startmask != 0xff)	 		/* do the first byte */
		if (xcnt == 0)			/* only one byte to do */
			endmask &= startmask;
		else {
			xcnt--;
			outb(VGA_GRAPH+1, startmask);
			*paddr++ |= 1;
		}
			
	if (endmask == 0xff)
		xcnt++;
	else {
		outb(VGA_GRAPH+1, endmask);	/* do final byte */
		*(paddr+xcnt) |= 1;
	}

	if (xcnt) {
		outb(VGA_GRAPH+1, 0xff);	/* do middle bytes */
		if (vga_gs->mode == GXcopy)
			vga_solidn(paddr, xcnt, 1);
		else
			vga_setn(paddr, xcnt, 1);
	}
}



/*
 *	vga_line_bres(pt1, pt2)		-- draw a one bit sloped line from
 *					pt1 to pt2 using the current linestyle
 *					using Bresenham's algorithm.  Pt1 is 
 *					guarenteed to be on the "left".
 *
 *	Input:
 *		DDXPointRec	pt1		-- first point in line
 *		DDXPointRec	pt2		-- last point in line
 */
vga_line_bres(pt1, pt2)
DDXPointRec	pt1;
DDXPointRec	pt2;
{
	BYTE *paddr;
	unsigned short mask;
	register short dx, dy;
	short incy;
	
	paddr = vga_fb + vga_byteoffset(pt1.x, pt1.y);
	mask = vga_bitvalue(pt1.x) << 8;

	outw(VGA_GRAPH, BITMASK | mask);

	*paddr |= 1;				/* draw first point */
	
	dx = pt2.x - pt1.x;			/* set up constants */
	dy = pt2.y - pt1.y;

	if (pt2.y > pt1.y)			/* set up y increment */
		incy = vga_slbytes;
	else {
		incy = -vga_slbytes;
		dy = -dy;
	}

	if (vt_info.is_vga) {
		outw(VGA_GRAPH, BITMASK | 0xff00);
		outw(VGA_GRAPH, gr_mode | 0x300);

		if (dy > dx) 				/* |slope| > 1 */
			vga_bres_high(paddr, mask, dy, (dx-dy)<<1, dx<<1, 
				      incy, (dx<<1) - dy);
		else
			vga_bres_low(paddr, mask, dx, (dy-dx)<<1, dy<<1, 
				     incy, (dy<<1)-dx);

		outw(VGA_GRAPH, gr_mode);
	}
	else {
		if (dy > dx) 				/* |slope| > 1 */
			ega_bres_high(paddr, mask, dy, (dx-dy)<<1, dx<<1, 
				      incy, (dx<<1) - dy);
		else
			ega_bres_low(paddr, mask, dx, (dy-dx)<<1, dy<<1, 
				     incy, (dy<<1)-dx);
	}
}
