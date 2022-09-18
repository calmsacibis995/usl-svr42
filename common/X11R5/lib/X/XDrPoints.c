/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDrPoints.c	1.1"
/* $XConsortium: XDrPoints.c,v 1.15 91/01/06 11:45:17 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XDrawPoints(dpy, d, gc, points, n_points, mode)
    register Display *dpy;
    Drawable d;
    GC gc;
    XPoint *points;
    int n_points;
    int mode; /* CoordMode */
{
    register xPolyPointReq *req;
    register long nbytes;
    int n;
    int xoff, yoff;
    XPoint pt;

    xoff = yoff = 0;
    LockDisplay(dpy);
    FlushGC(dpy, gc);
    while (n_points) {
	GetReq(PolyPoint, req);
	req->drawable = d;
	req->gc = gc->gid;
	req->coordMode = mode;
	n = n_points;
	if (n > (dpy->max_request_size - req->length))
	    n = dpy->max_request_size - req->length;
	req->length += n;
	nbytes = ((long)n) << 2; /* watch out for macros... */
	if (xoff || yoff) {
	    pt.x = xoff + points->x;
	    pt.y = yoff + points->y;
	    Data16 (dpy, (short *) &pt, 4);
	    if (nbytes > 4) {
		Data16 (dpy, (short *) (points + 1), nbytes - 4);
	    }
	} else {
	    Data16 (dpy, (short *) points, nbytes);
	}
	n_points -= n;
	if (n_points && (mode == CoordModePrevious)) {
	    register XPoint *pptr = points;
	    points += n;
	    while (pptr != points) {
		xoff += pptr->x;
		yoff += pptr->y;
		pptr++;
	    }
	} else
	    points += n;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
