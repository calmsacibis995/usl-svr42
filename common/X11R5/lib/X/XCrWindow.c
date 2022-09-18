/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XCrWindow.c	1.1"
/* $XConsortium: XCrWindow.c,v 11.13 91/01/06 11:45:00 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

Window XCreateSimpleWindow(dpy, parent, x, y, width, height, 
                      borderWidth, border, background)
    register Display *dpy;
    Window parent;
    int x, y;
    unsigned int width, height, borderWidth;
    unsigned long border;
    unsigned long background;
{
    Window wid;
    register xCreateWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(CreateWindow, 8, req);
    req->parent = parent;
    req->x = x;
    req->y = y;
    req->width = width;
    req->height = height;
    req->borderWidth = borderWidth;
    req->depth = 0;
    req->class = CopyFromParent;
    req->visual = CopyFromParent;
    wid = req->wid = XAllocID(dpy);
    req->mask = CWBackPixel | CWBorderPixel;

#ifdef MUSTCOPY
    {
	unsigned long lbackground = background, lborder = border;
	dpy->bufptr -= 8;
	Data32 (dpy, (long *) &lbackground, 4);
	Data32 (dpy, (long *) &lborder, 4);
    }
#else
    {
	register unsigned long *valuePtr =
	  (unsigned long *) NEXTPTR(req,xCreateWindowReq);
	*valuePtr++ = background;
	*valuePtr = border;
    }
#endif /* MUSTCOPY */

    UnlockDisplay(dpy);
    SyncHandle();
    return (wid);
    }
