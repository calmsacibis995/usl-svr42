/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetClOrig.c	1.1"
/* $XConsortium: XSetClOrig.c,v 11.13 91/01/06 11:47:55 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetClipOrigin (dpy, gc, xorig, yorig)
register Display *dpy;
GC gc;
int xorig, yorig;
{
    XGCValues *gv = &gc->values;

    LockDisplay(dpy);
    if (xorig != gv->clip_x_origin) {
        gv->clip_x_origin = xorig;
	gc->dirty |= GCClipXOrigin;
    }
    if (yorig != gv->clip_y_origin) {
        gv->clip_y_origin = yorig;
	gc->dirty |= GCClipYOrigin;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
