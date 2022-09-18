/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDelProp.c	1.1"
/* $XConsortium: XDelProp.c,v 11.6 91/01/06 11:45:05 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XDeleteProperty(dpy, window, property)
register Display *dpy;
Window window;
Atom property;
{
    register xDeletePropertyReq *req;

    LockDisplay(dpy);
    GetReq(DeleteProperty, req);
    req->window = window;
    req->property = property;
    UnlockDisplay(dpy);
    SyncHandle();
}
