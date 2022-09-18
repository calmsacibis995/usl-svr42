/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XFSSaver.c	1.1"
/* $XConsortium: XFSSaver.c,v 1.5 91/01/06 11:45:26 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1987	*/

/*
*/

#include "Xlibint.h"

XActivateScreenSaver(dpy) 
    register Display *dpy;

{
    XForceScreenSaver (dpy, ScreenSaverActive);
}

XResetScreenSaver(dpy) 
    register Display *dpy;

{
    XForceScreenSaver (dpy, ScreenSaverReset);
}

XForceScreenSaver(dpy, mode)
    register Display *dpy; 
    int mode;

{
    register xForceScreenSaverReq *req;

    LockDisplay(dpy);
    GetReq(ForceScreenSaver, req);
    req->mode = mode;
    UnlockDisplay(dpy);
    SyncHandle();
}

