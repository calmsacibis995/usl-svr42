/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XUnmapWin.c	1.1"
/* $XConsortium: XUnmapWin.c,v 11.7 91/01/06 11:48:44 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"
XUnmapWindow (dpy, w)
        register Display *dpy;
	Window w;
{
        register xResourceReq *req;

        LockDisplay(dpy);
        GetResReq(UnmapWindow, w, req);
	UnlockDisplay(dpy);
	SyncHandle();
}

