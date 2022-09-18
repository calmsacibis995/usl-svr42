/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetTSOrig.c	1.1"
/* $XConsortium: XSetTSOrig.c,v 11.4 91/01/06 11:48:15 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetTSOrigin (dpy, gc, x, y)
register Display *dpy;
GC gc;
int x, y;
{
    XGCValues *gv = &gc->values;

    LockDisplay(dpy);
    if (x != gv->ts_x_origin) {
	gv->ts_x_origin = x;
	gc->dirty |= GCTileStipXOrigin;
    }
    if (y != gv->ts_y_origin) {
	gv->ts_y_origin = y;
	gc->dirty |= GCTileStipYOrigin;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
