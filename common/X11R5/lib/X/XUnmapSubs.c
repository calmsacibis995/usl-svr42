/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XUnmapSubs.c	1.1"
/* $XConsortium: XUnmapSubs.c,v 11.8 91/01/06 11:48:43 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XUnmapSubwindows(dpy, win)
register Display *dpy;
Window win;
{
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(UnmapSubwindows,win, req);
    UnlockDisplay(dpy);
    SyncHandle();
}
