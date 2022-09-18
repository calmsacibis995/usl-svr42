/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XPmapBgnd.c	1.1"
/* $XConsortium: XPmapBgnd.c,v 11.8 91/01/06 11:47:17 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetWindowBackgroundPixmap(dpy, w, pixmap)
    register Display *dpy;
    Window w;
    Pixmap pixmap;
{
    register xChangeWindowAttributesReq *req;
    LockDisplay (dpy);
    GetReqExtra (ChangeWindowAttributes, 4, req);
    req->window = w;
    req->valueMask = CWBackPixmap;
    OneDataCard32 (dpy, NEXTPTR(req,xChangeWindowAttributesReq), pixmap);
    UnlockDisplay (dpy);
    SyncHandle();
}

