/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XConfWind.c	1.1"
/* $XConsortium: XConfWind.c,v 11.10 91/01/06 11:44:40 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XMoveResizeWindow(dpy, w, x, y, width, height)
register Display *dpy;
Window w;
int x, y;
unsigned int width, height;
{
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 16, req);
    req->window = w;
    req->mask = CWX | CWY | CWWidth | CWHeight;
#ifdef MUSTCOPY
    {
	long lx = x, ly = y;
	unsigned long lwidth = width, lheight = height;

	dpy->bufptr -= 16;
	Data32 (dpy, (long *) &lx, 4);	/* order must match values of */
	Data32 (dpy, (long *) &ly, 4);	/* CWX, CWY, CWWidth, and CWHeight */
	Data32 (dpy, (long *) &lwidth, 4);
	Data32 (dpy, (long *) &lheight, 4);
    }
#else
    {
	register unsigned long *valuePtr =
	  (unsigned long *) NEXTPTR(req,xConfigureWindowReq);
	*valuePtr++ = x;
	*valuePtr++ = y;
	*valuePtr++ = width;
	*valuePtr   = height;
    }
#endif /* MUSTCOPY */
    UnlockDisplay(dpy);
    SyncHandle();
}
