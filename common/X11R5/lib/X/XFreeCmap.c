/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XFreeCmap.c	1.1"
/* $XConsortium: XFreeCmap.c,v 11.10 91/05/13 22:51:31 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

extern void _XcmsDeleteCmapRec();

XFreeColormap(dpy, cmap)
register Display *dpy;
Colormap cmap;
{
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(FreeColormap, cmap, req);

    UnlockDisplay(dpy);
    SyncHandle();

    _XcmsDeleteCmapRec(dpy, cmap);
}
