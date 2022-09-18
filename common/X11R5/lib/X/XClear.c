/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XClear.c	1.1"
/* $XConsortium: XClear.c,v 11.8 91/01/06 11:44:38 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XClearWindow(dpy, w)
    register Display *dpy;
    Window w;
{
    register xClearAreaReq *req;

    LockDisplay(dpy);
    GetReq(ClearArea, req);
    req->window = w;
    req->x = req->y = req->width = req->height = 0;
        /* these values mean "clear the entire window" */
    req->exposures = xFalse;
    UnlockDisplay(dpy);
    SyncHandle();
}

