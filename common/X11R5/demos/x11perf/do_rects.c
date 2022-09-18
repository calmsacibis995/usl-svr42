/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5x11perf:do_rects.c	1.1"
/*****************************************************************************
Copyright 1988, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved



******************************************************************************/

#include "x11perf.h"
#include "bitmaps.h"

static XRectangle   *rects;
static GC	    pgc;

int InitRectangles(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int i;
    int size = p->special;
    int step;
    int x, y;
    int rows;
    int	half;
    int	lw = 0;

    pgc = xp->fggc;

    if (p->bfont)
    {
	lw = atoi (p->bfont);

	XSetLineAttributes(xp->d, xp->bggc, lw, LineSolid, CapButt, JoinMiter);
	XSetLineAttributes(xp->d, xp->fggc, lw, LineSolid, CapButt, JoinMiter);
	lw = (lw >> 1) + 1;
    }

    rects = (XRectangle *)malloc(p->objects * sizeof(XRectangle));
    x = lw;
    y = lw;
    rows = 0;
    if (xp->pack) {
	/* Pack rectangles as close as possible, mainly for debugging faster
	   tiling, stippling routines in a server */
	step = size;
    } else {
	/* Try to exercise all alignments...any odd number is okay */
	step = size + 1 + (size % 2);
    }

    for (i = 0; i != p->objects; i++) {
	rects[i].x = x;
        rects[i].y = y;
	rects[i].width = rects[i].height = size;

	y += step;
	rows++;
	if (y + size > HEIGHT || rows == MAXROWS) {
	    rows = 0;
	    y = lw;
	    x += step;
	    if (x + size > WIDTH) {
		x = lw;
	    }
	}
    }

    SetFillStyle(xp, p);

    return reps;
}

void DoRectangles(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int i;

    for (i = 0; i != reps; i++) {
        XFillRectangles(xp->d, xp->w, pgc, rects, p->objects);
        if (pgc == xp->bggc)
            pgc = xp->fggc;
        else
            pgc = xp->bggc;
    }
}

void DoOutlineRectangles (xp, p, reps)
    XParms  xp;
    Parms   p;
    int	    reps;
{
    int	i;

    for (i = 0; i != reps; i++) {
	XDrawRectangles (xp->d, xp->w, pgc, rects, p->objects);
        if (pgc == xp->bggc)
            pgc = xp->fggc;
        else
            pgc = xp->bggc;
    }
}

void EndRectangles(xp, p)
    XParms  xp;
    Parms p;
{
    free(rects);
}

