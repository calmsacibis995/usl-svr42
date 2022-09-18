/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XRepWindow.c	1.1"
/* $XConsortium: XRepWindow.c,v 11.7 91/01/06 11:47:47 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XReparentWindow(dpy, w, p, x, y)
    register Display *dpy;
    Window w, p;
    int x, y;
{
    register xReparentWindowReq *req;

    LockDisplay(dpy);
    GetReq(ReparentWindow, req);
    req->window = w;
    req->parent = p;
    req->x = x;
    req->y = y;
    UnlockDisplay(dpy);
    SyncHandle();
}

