/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDrRects.c	1.1"
/* $XConsortium: XDrRects.c,v 11.14 91/01/06 11:45:20 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XDrawRectangles(dpy, d, gc, rects, n_rects)
register Display *dpy;
Drawable d;
GC gc;
XRectangle *rects;
int n_rects;
{
    register xPolyRectangleReq *req;
    long len;
    int n;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    while (n_rects) {
	GetReq(PolyRectangle, req);
	req->drawable = d;
	req->gc = gc->gid;
	n = n_rects;
	len = ((long)n) << 1;
	if (len > (dpy->max_request_size - req->length)) {
	    n = (dpy->max_request_size - req->length) >> 1;
	    len = ((long)n) << 1;
	}
	req->length += len;
	len <<= 2; /* watch out for macros... */
	Data16 (dpy, (short *) rects, len);
	n_rects -= n;
	rects += n;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
