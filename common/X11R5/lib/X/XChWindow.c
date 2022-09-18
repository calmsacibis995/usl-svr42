/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XChWindow.c	1.1"
/* $XConsortium: XChWindow.c,v 11.10 91/01/06 11:44:26 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XResizeWindow(dpy, w, width, height)
register Display *dpy;
Window w;
unsigned int width, height;
{
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 8, req); /* 2 4-byte quantities */

    /* XXX assuming that sizeof(unsigned long) is 32 bits */

    req->window = w;
    req->mask = CWWidth | CWHeight;
#ifdef MUSTCOPY
    {
	unsigned long lwidth = width, lheight = height;
    dpy->bufptr -= 8;
    Data32 (dpy, (long *) &lwidth, 4);	/* order dictated by values of */
    Data32 (dpy, (long *) &lheight, 4);	/* CWWidth and CWHeight */
    }
#else
    {
	unsigned long * valuePtr =
	  (unsigned long *) NEXTPTR(req,xConfigureWindowReq);
	*valuePtr++ = width;
	*valuePtr++ = height;
    }
#endif /* MUSTCOPY */
    UnlockDisplay(dpy);
    SyncHandle();
}
