/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDrLine.c	1.1"
/* $XConsortium: XDrLine.c,v 11.15 91/01/06 11:45:13 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

/* precompute the maximum size of batching request allowed */

#define wsize (SIZEOF(xPolySegmentReq) + WLNSPERBATCH * SIZEOF(xSegment))
#define zsize (SIZEOF(xPolySegmentReq) + ZLNSPERBATCH * SIZEOF(xSegment))

XDrawLine (dpy, d, gc, x1, y1, x2, y2)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x1, y1, x2, y2;
{
    register xSegment *segment;
#ifdef MUSTCOPY
    xSegment segmentdata;
    long len = SIZEOF(xSegment);

    segment = &segmentdata;
#endif /* not MUSTCOPY */

    LockDisplay(dpy);
    FlushGC(dpy, gc);

    {
    register xPolySegmentReq *req = (xPolySegmentReq *) dpy->last_req;

    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolySegment)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && ((dpy->bufptr + SIZEOF(xSegment)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < (gc->values.line_width ?
						  wsize : zsize)) ) {
	 req->length += SIZEOF(xSegment) >> 2;
#ifndef MUSTCOPY
         segment = (xSegment *) dpy->bufptr;
	 dpy->bufptr += SIZEOF(xSegment);
#endif /* not MUSTCOPY */
	 }

    else {
	GetReqExtra (PolySegment, SIZEOF(xSegment), req);
	req->drawable = d;
	req->gc = gc->gid;
#ifdef MUSTCOPY
	dpy->bufptr -= SIZEOF(xSegment);
#else
	segment = (xSegment *) NEXTPTR(req,xPolySegmentReq);
#endif /* MUSTCOPY */
	}

    segment->x1 = x1;
    segment->y1 = y1;
    segment->x2 = x2;
    segment->y2 = y2;

#ifdef MUSTCOPY
    Data (dpy, (char *) &segmentdata, len);
#endif /* MUSTCOPY */

    UnlockDisplay(dpy);
    SyncHandle();
    }
}

