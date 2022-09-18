/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XCrCmap.c	1.1"
/* $XConsortium: XCrCmap.c,v 11.11 91/02/12 16:10:51 dave Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

Colormap XCreateColormap(dpy, w, visual, alloc)
register Display *dpy;
Window w;
Visual *visual;
int alloc;
{
    register xCreateColormapReq *req;
    Colormap mid;

    LockDisplay(dpy);
    GetReq(CreateColormap, req);
    req->window = w;
    mid = req->mid = XAllocID(dpy);
    req->alloc = alloc;
    if (visual == CopyFromParent) req->visual = CopyFromParent;
    else req->visual = visual->visualid;

    UnlockDisplay(dpy);
    SyncHandle();

    _XcmsAddCmapRec(dpy, mid, w, visual);

    return(mid);
}
