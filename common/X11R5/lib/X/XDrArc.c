/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDrArc.c	1.1"
/* $XConsortium: XDrArc.c,v 11.15 91/01/06 11:45:10 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

/* Note to future maintainers:  XDrawArc does NOT batch successive PolyArc
   requests into a single request like XDrawLine, XDrawPoint, etc.
   We don't do this because X_PolyArc applies the GC's join-style if
   the last point in one arc coincides with the first point in another.
   The client wouldn't expect this and would have no easy way to defeat it. */
   
#include "Xlibint.h"

XDrawArc(dpy, d, gc, x, y, width, height, angle1, angle2)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y; /* INT16 */
    unsigned int width, height; /* CARD16 */
    int angle1, angle2; /* INT16 */
{
    register xPolyArcReq *req;
    register xArc *arc;
#ifdef MUSTCOPY
    xArc arcdata;
    long len = SIZEOF(xArc);

    arc = &arcdata;
#endif /* MUSTCOPY */

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReqExtra (PolyArc, SIZEOF(xArc), req);

    req->drawable = d;
    req->gc = gc->gid;

#ifndef MUSTCOPY
    arc = (xArc *) NEXTPTR(req,xPolyArcReq);
#endif /* MUSTCOPY */

    arc->x = x;
    arc->y = y;
    arc->width = width;
    arc->height = height;
    arc->angle1 = angle1;
    arc->angle2 = angle2;

#ifdef MUSTCOPY
    dpy->bufptr -= SIZEOF(xArc);
    Data (dpy, (char *) arc, len);
#endif /* MUSTCOPY */

    UnlockDisplay(dpy);
    SyncHandle();
}
