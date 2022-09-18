/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XChCmap.c	1.1"
/* $XConsortium: XChCmap.c,v 11.8 91/01/06 11:44:18 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetWindowColormap(dpy, w, colormap)
    register Display *dpy;
    Window w;
    Colormap colormap;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReqExtra (ChangeWindowAttributes, 4, req);
    req->window = w;
    req->valueMask = CWColormap;
    OneDataCard32 (dpy, NEXTPTR(req,xChangeWindowAttributesReq), colormap);
    UnlockDisplay(dpy);
    SyncHandle();
}

