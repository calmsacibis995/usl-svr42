/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XChGC.c	1.1"
/* $XConsortium: XChGC.c,v 11.10 91/01/06 11:44:19 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XChangeGC (dpy, gc, valuemask, values)
    register Display *dpy;
    GC gc;
    unsigned long valuemask;
    XGCValues *values;
{
    LockDisplay(dpy);

    valuemask &= (1L << (GCLastBit + 1)) - 1;
    if (valuemask) _XUpdateGCCache (gc, valuemask, values);

    /* if any Resource ID changed, must flush */
    if (gc->dirty & (GCFont | GCTile | GCStipple))
	_XFlushGCCache(dpy, gc);
    UnlockDisplay(dpy);
    SyncHandle();
}

