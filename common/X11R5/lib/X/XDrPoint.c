/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDrPoint.c	1.1"
/* $XConsortium: XDrPoint.c,v 11.13 91/01/06 11:45:16 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

/* precompute the maximum size of batching request allowed */

#define size (SIZEOF(xPolyPointReq) + PTSPERBATCH * SIZEOF(xPoint))

XDrawPoint(dpy, d, gc, x, y)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y; /* INT16 */
{
    xPoint *point;
#ifdef MUSTCOPY
    xPoint pointdata;
    long len = SIZEOF(xPoint);

    point = &pointdata;
#endif /* MUSTCOPY */

    LockDisplay(dpy);
    FlushGC(dpy, gc);

    {
    register xPolyPointReq *req = (xPolyPointReq *) dpy->last_req;


    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolyPoint)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && (req->coordMode == CoordModeOrigin)
       && ((dpy->bufptr + SIZEOF(xPoint)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < size) ) {
	 req->length += SIZEOF(xPoint) >> 2;
#ifndef MUSTCOPY
         point = (xPoint *) dpy->bufptr;
	 dpy->bufptr += SIZEOF(xPoint);
#endif /* not MUSTCOPY */
	 }

    else {
	GetReqExtra(PolyPoint, 4, req); /* 1 point = 4 bytes */
	req->drawable = d;
	req->gc = gc->gid;
	req->coordMode = CoordModeOrigin;
#ifdef MUSTCOPY
	dpy->bufptr -= SIZEOF(xPoint);
#else
	point = (xPoint *) NEXTPTR(req,xPolyPointReq);
#endif /* MUSTCOPY */
	}

    point->x = x;
    point->y = y;

#ifdef MUSTCOPY
    Data (dpy, (char *) point, len);
#endif /* MUSTCOPY */
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

