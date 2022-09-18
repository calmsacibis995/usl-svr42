/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256lnclip.c	1.1"

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

#define	OUT_ABOVE	1		/* outcall defines */
#define	OUT_BELOW	2
#define	OUT_LEFT	4
#define	OUT_RIGHT	8



/*
 *	v256_clip_line(pt1, pt2)	-- clip the line defined by pt1 and pt2 using
 *				the current clipping region in v256_clip_?.
 *				The edges defined by the clipping region are
 *				included in the region.
 *
 *	Input:
 *		DDXPointRec	*pt1	-- pointer to first point on line
 *		DDXPointRec	*pt2	-- pointer to second point on line
 */
v256_clip_line(pt1, pt2)
DDXPointRec *pt1, *pt2;
{
	register BITS16	oc1;
	BITS16		oc2, tbits;
	int		in, out;
	register int	dx, dy;
	DDXPointRec	tpt;

	oc1 = v256_outcode(pt1->x, pt1->y);
	oc2 = v256_outcode(pt2->x, pt2->y);

	in = ((oc1 | oc2) == 0);
	out = ((oc1 & oc2) != 0);
	
	while (!in && !out) {
		if (oc1 == 0) {			/* swap so pt1 is clipped */
			v256_swap(*pt1, *pt2, tpt);
			v256_swap(oc1, oc2, tbits);
		}

		dx = pt2->x - pt1->x;
		dy = pt2->y - pt1->y;

		if (oc1 & OUT_LEFT) {
			pt1->y += (dy * (v256_clip_x1 - pt1->x)) / dx;
			pt1->x = v256_clip_x1;
		}
	
		else if (oc1 & OUT_RIGHT) {
			pt1->y += (dy * (v256_clip_x2 - pt1->x)) / dx;
			pt1->x = v256_clip_x2;
		}
	
		else if (oc1 & OUT_BELOW) {
			pt1->x += (dx * (v256_clip_y2 - pt1->y)) / dy;
			pt1->y = v256_clip_y2;
		}
	
		else if (oc1 & OUT_ABOVE) {
			pt1->x += (dx * (v256_clip_y1 - pt1->y)) / dy;
			pt1->y = v256_clip_y1;
		}
	
		oc1 = v256_outcode(pt1->x, pt1->y);
		in = ((oc1 | oc2) == 0);
		out = ((oc1 & oc2) != 0);
	}
	return(in);
}



/*
 *	v256_outcode(x, y) -- return the outcode values for the point (x, y)
 *			using the v256_clip_? clip region.
 *
 *	Input:
 *		int	x	-- x position of point
 *		int 	y	-- y position of poing
 */
v256_outcode(x, y)
register int x, y;
{
	register int	code;

	code = 0;
	if (y < v256_clip_y1) code |= OUT_ABOVE;
	if (y > v256_clip_y2) code |= OUT_BELOW;
	if (x < v256_clip_x1) code |= OUT_LEFT;
	if (x > v256_clip_x2) code |= OUT_RIGHT;
	return(code);
}
