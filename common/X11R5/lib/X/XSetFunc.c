/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetFunc.c	1.1"
/* $XConsortium: XSetFunc.c,v 11.9 91/01/06 11:48:01 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetFunction (dpy, gc, function)
register Display *dpy;
GC gc;
int function;
{
    LockDisplay(dpy);
    if (gc->values.function != function) {
	gc->values.function = function;
	gc->dirty |= GCFunction;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
