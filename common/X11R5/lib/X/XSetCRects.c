/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetCRects.c	1.1"
/* $XConsortium: XSetCRects.c,v 11.19 91/01/06 11:47:53 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetClipRectangles (dpy, gc, clip_x_origin, clip_y_origin, rectangles, n,
                    ordering)
    register Display *dpy;
    GC gc;
    int clip_x_origin, clip_y_origin;
    XRectangle *rectangles;
    int n;
    int ordering;
{
    LockDisplay(dpy);
    _XSetClipRectangles (dpy, gc, clip_x_origin, clip_y_origin, rectangles, n,
                    ordering);
    UnlockDisplay(dpy);
    SyncHandle();
}
    
/* can only call when display is locked. */
_XSetClipRectangles (dpy, gc, clip_x_origin, clip_y_origin, rectangles, n,
                    ordering)
    register Display *dpy;
    GC gc;
    int clip_x_origin, clip_y_origin;
    XRectangle *rectangles;
    int n;
    int ordering;
{
    register xSetClipRectanglesReq *req;
    register long nbytes;

    GetReq (SetClipRectangles, req);
    req->gc = gc->gid;
    req->xOrigin = gc->values.clip_x_origin = clip_x_origin;
    req->yOrigin = gc->values.clip_y_origin = clip_y_origin;
    req->ordering = ordering;
    req->length += n * (SIZEOF(xRectangle)/4);
    gc->rects = 1;
    nbytes = (long)n * SIZEOF(xRectangle);
    Data16 (dpy, (short *) rectangles, nbytes);
    gc->dirty &= ~(GCClipMask | GCClipXOrigin | GCClipYOrigin);
}
