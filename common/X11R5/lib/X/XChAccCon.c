/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XChAccCon.c	1.1"
/* $XConsortium: XChAccCon.c,v 11.9 91/01/06 11:44:15 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XEnableAccessControl(dpy) 
    register Display *dpy;

{
    XSetAccessControl(dpy, EnableAccess);
}

XDisableAccessControl(dpy) 
    register Display *dpy;

{
    XSetAccessControl(dpy, DisableAccess);
}

XSetAccessControl(dpy, mode)
    register Display *dpy; 
    int mode;

{
    register xSetAccessControlReq *req;

    LockDisplay(dpy);
    GetReq(SetAccessControl, req);
    req->mode = mode;
    UnlockDisplay(dpy);
    SyncHandle();
}

