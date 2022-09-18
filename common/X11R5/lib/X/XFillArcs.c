/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XFillArcs.c	1.1"
/* $XConsortium: XFillArcs.c,v 11.15 91/01/06 11:45:32 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

#define arc_scale (SIZEOF(xArc) / 4)

XFillArcs(dpy, d, gc, arcs, n_arcs)
register Display *dpy;
Drawable d;
GC gc;
XArc *arcs;
int n_arcs;
{
    register xPolyFillArcReq *req;
    long len;
    int n;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    while (n_arcs) {
	GetReq(PolyFillArc, req);
	req->drawable = d;
	req->gc = gc->gid;
	n = n_arcs;
	len = ((long)n) * arc_scale;
	if (len > (dpy->max_request_size - req->length)) {
	    n = (dpy->max_request_size - req->length) / arc_scale;
	    len = ((long)n) * arc_scale;
	}
	req->length += len;
	len <<= 2; /* watch out for macros... */
	Data16 (dpy, (short *) arcs, len);
	n_arcs -= n;
	arcs += n;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
