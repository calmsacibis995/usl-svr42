/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XCopyCmap.c	1.1"
/* $XConsortium: XCopyCmap.c,v 11.9 91/02/12 16:10:45 dave Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

Colormap XCopyColormapAndFree(dpy, src_cmap)
register Display *dpy;
Colormap src_cmap;
{
    Colormap mid;
    register xCopyColormapAndFreeReq *req;

    LockDisplay(dpy);
    GetReq(CopyColormapAndFree, req);

    mid = req->mid = XAllocID(dpy);
    req->srcCmap = src_cmap;

    UnlockDisplay(dpy);
    SyncHandle();

    _XcmsCopyCmapRecAndFree(dpy, src_cmap, mid);

    return(mid);
}
