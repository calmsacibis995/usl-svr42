/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XFillPoly.c	1.1"
/* $XConsortium: XFillPoly.c,v 11.13 91/01/06 11:45:34 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XFillPolygon(dpy, d, gc, points, n_points, shape, mode)
register Display *dpy;
Drawable d;
GC gc;
XPoint *points;
int n_points;
int shape;
int mode;
{
    register xFillPolyReq *req;
    register long nbytes;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(FillPoly, req);

    req->drawable = d;
    req->gc = gc->gid;
    req->shape = shape;
    req->coordMode = mode;

    if ((req->length + n_points) > (unsigned)65535)
	n_points = 65535 - req->length; /* force BadLength, if possible */
    req->length += n_points;

    /* shift (mult. by 4) before passing to the (possible) macro */

    nbytes = n_points << 2;
    
    Data16 (dpy, (short *) points, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
}
