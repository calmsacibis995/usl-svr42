/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDestWind.c	1.1"
/* $XConsortium: XDestWind.c,v 11.7 91/01/06 11:45:07 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"
XDestroyWindow (dpy, w)
	register Display *dpy;
	Window w;
{
        register xResourceReq *req;

	LockDisplay(dpy);
        GetResReq(DestroyWindow, w, req);
	UnlockDisplay(dpy);
	SyncHandle();
}

