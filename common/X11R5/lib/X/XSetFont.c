/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetFont.c	1.1"
/* $XConsortium: XSetFont.c,v 11.9 91/01/06 11:47:59 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetFont (dpy, gc, font)
register Display *dpy;
GC gc;
Font font;
{
    LockDisplay(dpy);
    if (gc->values.font != font) {
        gc->values.font = font;
	gc->dirty |= GCFont;
	_XFlushGCCache(dpy, gc);
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
