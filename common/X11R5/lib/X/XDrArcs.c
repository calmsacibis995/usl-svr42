/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDrArcs.c	1.1"
/* $XConsortium: XDrArcs.c,v 11.17 91/01/06 11:45:12 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

#define arc_scale (SIZEOF(xArc) / 4)

XDrawArcs(dpy, d, gc, arcs, n_arcs)
register Display *dpy;
Drawable d;
GC gc;
XArc *arcs;
int n_arcs;
{
    register xPolyArcReq *req;
    register long len;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(PolyArc,req);
    req->drawable = d;
    req->gc = gc->gid;
    len = ((long)n_arcs) * arc_scale;
    if ((req->length + len) > (unsigned)65535)
	len = 1; /* force BadLength */
    req->length += len;
    len <<= 2; /* watch out for macros... */
    Data16 (dpy, (short *) arcs, len);
    UnlockDisplay(dpy);
    SyncHandle();
}
