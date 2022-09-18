/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sizerdline.c	1.3"

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

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: cfbline.c,v 1.13 90/01/23 15:14:21 keith Exp $ */

#include "X.h"

#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "mistruct.h"
#include "si.h"
#include "sidep.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

/* single-pixel lines on a color frame buffer

   NON-SLOPED LINES
   horizontal lines are always drawn left to right; we have to
move the endpoints right by one after they're swapped.
   horizontal lines will be confined to a single band of a
region.  the code finds that band (giving up if the lower
bound of the band is above the line we're drawing); then it
finds the first box in that band that contains part of the
line.  we clip the line to subsequent boxes in that band.
   vertical lines are always drawn top to bottom (y-increasing.)
this requires adding one to the y-coordinate of each endpoint
after swapping.

   SLOPED LINES
   when clipping a sloped line, we bring the second point inside
the clipping box, rather than one beyond it, and then add 1 to
the length of the line before drawing it.  this lets us use
the same box for finding the outcodes for both endpoints.  since
the equation for clipping the second endpoint to an edge gives us
1 beyond the edge, we then have to move the point towards the
first point by one step on the major axis.
   eventually, there will be a diagram here to explain what's going
on.  the method uses Cohen-Sutherland outcodes to determine
outsideness, and a method similar to Pike's layers for doing the
actual clipping.

*/

/* out of clip region codes */
#define OUT_LEFT 0x08
#define OUT_RIGHT 0x04
#define OUT_ABOVE 0x02
#define OUT_BELOW 0x01

/* major axis for bresenham's line */
#define X_AXIS	0
#define Y_AXIS	1

#define OUTCODES(result, x, y, pbox) \
    if (x < pbox->x1) \
	result |= OUT_LEFT; \
    else if (x >= pbox->x2) \
	result |= OUT_RIGHT; \
    if (y < pbox->y1) \
	result |= OUT_ABOVE; \
    else if (y >= pbox->y2) \
	result |= OUT_BELOW;

/* Dashed bresenham line */
#define StepDash\
    if (!--dashRemaining) { \
	if (++ dashIndex == numInDashList) \
	    dashIndex = 0; \
	dashRemaining = pDash[dashIndex]; \
	if (!isDoubleDash) \
	    dontdraw = (dashIndex & 1); \
    }

/*
 * Generate the points to be drawn in a dashed 0 width line.
 */
siBresD(pdashIndex, pDash, numInDashList, pdashOffset, isDoubleDash,
	 x1, y1, x2, y2, evenPts, oddPts)
int *pdashIndex;	/* current dash */
unsigned char *pDash;	/* dash list */
int numInDashList;	/* total length of dash list */
int *pdashOffset;	/* offset into current dash */
int isDoubleDash;
int x1, y1, x2, y2;	/* line endpoints */
register DDXPointPtr *evenPts, *oddPts;
{
    int e3;
    register int x, y;
    int dashIndex;
    int dashOffset;
    int dashRemaining;
    int dontdraw;
    int adx, ady, signdx, signdy, e, e1, e2, len, axis;

    adx = x2 - x1;
    ady = y2 - y1;
    signdx = sign(adx);
    signdy = sign(ady);
    adx = abs(adx);
    ady = abs(ady);

    if (adx > ady) {
	axis = X_AXIS;
	e1 = ady << 1;
	e2 = e1 - (adx << 1);
	e = e1 - adx;
	len = adx;
    }
    else {
	axis = Y_AXIS;
	e1 = adx << 1;
	e2 = e1 - (ady << 1);
	e = e1 - ady;
	len = ady;
    }

    e3 = e2-e1;

    dashOffset = *pdashOffset;
    dashIndex = *pdashIndex;
    dashRemaining = pDash[dashIndex] - dashOffset;
    if (isDoubleDash)
    {
	dontdraw = 0;
    }
    else
	dontdraw = (dashIndex & 1);

    /* point to first point */
    x = x1;
    y = y1;
    e = e-e1;			/* to make looping easier */
    
    if (axis == X_AXIS) {
	while(len--) { 
	    if (!dontdraw) {
		if (dashIndex & 1) {
		    (*oddPts)->x = x;
		    (*oddPts)->y = y;
		    (*oddPts)++;
		}
		else {
		    (*evenPts)->x = x;
		    (*evenPts)->y = y;
		    (*evenPts)++;
		}
	    }
	    e += e1;
	    if (e >= 0) {
		y += signdy;
		e += e3;
	    }
	    x += signdx;
	    StepDash
	}
    }
    else {
	while(len--) {
	    if (!dontdraw) {
		if (dashIndex & 1) {
		    (*oddPts)->x = x;
		    (*oddPts)->y = y;
		    (*oddPts)++;
		}
		else {
		    (*evenPts)->x = x;
		    (*evenPts)->y = y;
		    (*evenPts)++;
		}
	    }
	    e += e1;
	    if (e >= 0) {
		x += signdx;
		e += e3;
	    }
	    y += signdy;
	    StepDash
    	}
    }

    *pdashIndex = dashIndex;
    *pdashOffset = pDash[dashIndex] - dashRemaining;
}


siZeroDashLine(pDrawable, pGC, mode, npt, pptInit)
DrawablePtr	pDrawable;
register 	GCPtr pGC;
int		mode;		/* Origin or Previous */
int		npt;		/* number of points */
DDXPointPtr	pptInit;
{
    register DDXPointPtr endpts;/* translated endpoints */
    unsigned int oc1;		/* outcode of point 1 */
    unsigned int oc2;		/* outcode of point 2 */
    int n;			/* length of segment */
    int x1, x2, y1, y2;
    int	direct;
    int	xorg, yorg;
    unsigned char   *pDash;
    int	dashOffset;
    int	numInDashList;
    int	dashIndex;
    int	isDoubleDash;
    int	dashIndexTmp, dashOffsetTmp;
    int maxw, i;
    DDXPointPtr points, pts, oddPts;
    int numPts = 0;
    int *widths;
    unsigned long fgPixel = pGC->fgPixel;

    /*
     * Find out the longest line that will be drawn
     */
    for (pts = pptInit+1, i = npt-1; --i >= 0; pts++) {
	n = max(abs(pts->x - (pts-1)->x), abs(pts->y - (pts-1)->y)) + 1;
	if (n > numPts)
	    numPts = n;
    }

    if (!numPts)
	return;

    /*
     * Set up point and width arrays for filling in later.
     */
    widths = (int *)ALLOCATE_LOCAL(sizeof(int) * (numPts << 1));
    if (!widths)
	return;
    maxw = 0;

    points = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * (numPts << 1));
    if (!points) {
	DEALLOCATE_LOCAL(widths);
	return;
    }

    /*
     * Find out the offset for the drawable
     */
    if (pGC->miTranslate && (pDrawable->type == DRAWABLE_WINDOW)) {
	xorg = pDrawable->x;	/* SI (R4) */
	yorg = pDrawable->y;	/* SI (R4) */
    }
    else {
	xorg = 0;
	yorg = 0;
    }

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, npt, pptInit);
    }
#endif

    /* 
     * compute initial dash values
     */
    pDash = (unsigned char *) pGC->dash;
    numInDashList = pGC->numInDashList;
    isDoubleDash = (pGC->lineStyle == LineDoubleDash);
    dashIndex = 0;
    dashOffset = 0;
    miStepDash ((int)pGC->dashOffset, &dashIndex, pDash,
		    numInDashList, &dashOffset);

    /*
     * Loop through the lines, drawing them the best way possible
     */
    endpts = pptInit;
    x2 = endpts->x + xorg;
    y2 = endpts->y + yorg;
    while(--npt) {
	x1 = x2;
	y1 = y2;
	++endpts;

	if (mode == CoordModePrevious) {
	    xorg = x1;
	    yorg = y1;
	}
	x2 = endpts->x + xorg;
	y2 = endpts->y + yorg;

	/*
	 * See if we can use an sdd point plotting routine.  To do this, 
	 * we can't need to clip, and must be drawing FillSolid.
	 */
	direct = 0;
	if ((pGC->fillStyle == FillSolid) && 
	    pDrawable->type == DRAWABLE_WINDOW && si_hasplotpoint) {
	    register 	int nbox;
	    register 	BoxPtr pbox;
	    RegionPtr	prgnDst;

	    prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;		/* SI (R4) */
	    pbox = REGION_RECTS(prgnDst);	/* SI (R4) */
	    nbox = REGION_NUM_RECTS(prgnDst);	/* SI (R4) */
	    if (nbox == 0) {			/* nothing to draw */
		DEALLOCATE_LOCAL(widths);
		DEALLOCATE_LOCAL(points);
		return;
	    }

	    while(nbox--) {
		oc1 = 0;
		oc2 = 0;
		OUTCODES(oc1, x1, y1, pbox);
		OUTCODES(oc1, x2, y2, pbox);

		if ((oc1 | oc2) == 0)		/* no clipping required */
			direct = 1;
	    }
	    if (direct) {
		si_PrepareGS(pGC);
	    }
	}

	/* 
	 * Generate the points in the line and draw them.
	 */
	pts = points;
	oddPts = &points[numPts];
	siBresD(&dashIndex, pDash, numInDashList,
		&dashOffset, isDoubleDash,
		x1, y1, x2, y2, &pts, &oddPts);

	n = pts - points;
	if (n && (!direct || (si_plotpoints(n, points) == SI_FAIL))) {
	    if (n > maxw) {
		while (maxw < n)
		    widths[maxw++] = 1;
	    }
	    (*pGC->ops->FillSpans)(pDrawable, pGC, n, points, widths, FALSE);
	}

	if (pGC->lineStyle != LineDoubleDash)
	    continue;

	if ((pGC->fillStyle == FillSolid) ||
	    (pGC->fillStyle == FillStippled)) {
	    DoChangeGC(pGC, GCForeground, (XID *)&pGC->bgPixel, 0);
	    ValidateGC(pDrawable, pGC);
	    if (direct) {
		si_PrepareGS(pGC);
	    }
	}
	pts = &points[numPts];
	n = oddPts - pts;

	if (n && (!direct || (si_plotpoints(n, pts) == SI_FAIL))) {
	    if (n > maxw) {
		while (maxw < n)
		    widths[maxw++] = 1;
	    }
	    (*pGC->ops->FillSpans)(pDrawable, pGC, n, pts, widths, FALSE);
	}

	if ((pGC->fillStyle == FillSolid) ||
	    (pGC->fillStyle == FillStippled)) {
	    DoChangeGC(pGC, GCForeground, (XID *)&fgPixel, 0);
	    ValidateGC(pDrawable, pGC);
	    if (direct) {
		si_PrepareGS(pGC);
	    }
	}
    }
    DEALLOCATE_LOCAL(widths);
    DEALLOCATE_LOCAL(points);
}
