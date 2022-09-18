/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDrSegs.c	1.1"
/* $XConsortium: XDrSegs.c,v 11.13 91/01/06 11:45:21 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XDrawSegments (dpy, d, gc, segments, nsegments)
    register Display *dpy;
    Drawable d;
    GC gc;
    XSegment *segments;
    int nsegments;
{
    register xPolySegmentReq *req;
    long len;
    int n;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    while (nsegments) {
	GetReq (PolySegment, req);
	req->drawable = d;
	req->gc = gc->gid;
	n = nsegments;
	len = ((long)n) << 1;
	if (len > (dpy->max_request_size - req->length)) {
	    n = (dpy->max_request_size - req->length) >> 1;
	    len = ((long)n) << 1;
	}
	req->length += len;
	len <<= 2; /* watch out for macros... */
	Data16 (dpy, (short *) segments, len);
	nsegments -= n;
	segments += n;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

