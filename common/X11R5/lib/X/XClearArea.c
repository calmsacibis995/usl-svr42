/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XClearArea.c	1.1"
/* $XConsortium: XClearArea.c,v 11.9 91/01/06 11:44:39 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XClearArea (dpy, w, x, y, width, height, exposures)
    register Display *dpy;
    Window w;
    int x, y;
    unsigned int width, height;
    Bool exposures;
{
    register xClearAreaReq *req;

    LockDisplay(dpy);
    GetReq(ClearArea, req);
    req->window = w;
    req->x = x;
    req->y = y;
    req->width = width;
    req->height = height;
    req->exposures = exposures;
    UnlockDisplay(dpy);
    SyncHandle();
}

