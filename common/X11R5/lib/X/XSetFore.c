/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetFore.c	1.1"
/* $XConsortium: XSetFore.c,v 11.9 91/01/06 11:48:00 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetForeground (dpy, gc, foreground)
register Display *dpy;
GC gc;
unsigned long foreground; /* CARD32 */
{
    LockDisplay(dpy);
    if (gc->values.foreground != foreground) {
	gc->values.foreground = foreground;
	gc->dirty |= GCForeground;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
