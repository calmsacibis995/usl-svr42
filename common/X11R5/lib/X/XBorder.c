/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XBorder.c	1.1"
/* $XConsortium: XBorder.c,v 11.8 91/01/06 11:44:13 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetWindowBorder(dpy, w, pixel)
    register Display *dpy;
    Window w;
    unsigned long pixel;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReqExtra (ChangeWindowAttributes, 4, req);
    req->window = w;
    req->valueMask = CWBorderPixel;
    OneDataCard32 (dpy, NEXTPTR(req,xChangeWindowAttributesReq), pixel);
    UnlockDisplay(dpy);
    SyncHandle();
}

