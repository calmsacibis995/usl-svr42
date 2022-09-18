/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XMoveWin.c	1.1"
/* $XConsortium: XMoveWin.c,v 11.10 91/01/06 11:47:04 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XMoveWindow (dpy, w, x, y)
    register Display *dpy;
    Window w;
    int x, y;
{
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 8, req);

    req->window = w;
    req->mask = CWX | CWY;

#ifdef MUSTCOPY
    {
	long lx = (long) x, ly = (long) y;
	dpy->bufptr -= 8;
	Data32 (dpy, (long *) &lx, 4);	/* order dictated by CWX and CWY */
	Data32 (dpy, (long *) &ly, 4);
    }
#else
    {
	unsigned long *valuePtr =
	  (unsigned long *) NEXTPTR(req,xConfigureWindowReq);
	*valuePtr++ = x;
	*valuePtr = y;
    }
#endif /* MUSTCOPY */
    UnlockDisplay(dpy);
    SyncHandle();
}

