/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDestSubs.c	1.1"
/* $XConsortium: XDestSubs.c,v 11.6 91/01/06 11:45:06 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XDestroySubwindows(dpy, win)
register Display *dpy;
Window win;
{
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq (DestroySubwindows,win, req);
    UnlockDisplay(dpy);
    SyncHandle();
}

