/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetBack.c	1.1"
/* $XConsortium: XSetBack.c,v 11.9 91/01/06 11:47:52 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetBackground (dpy, gc, background)
register Display *dpy;
GC gc;
unsigned long background; /* CARD32 */
{
    LockDisplay(dpy);
    if (gc->values.background != background) {
	gc->values.background = background;
	gc->dirty |= GCBackground;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
