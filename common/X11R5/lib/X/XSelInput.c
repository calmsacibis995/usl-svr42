/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSelInput.c	1.1"
/* $XConsortium: XSelInput.c,v 11.9 91/01/06 11:47:50 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSelectInput (dpy, w, mask)
    register Display *dpy;
    Window w;
    long mask;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReqExtra (ChangeWindowAttributes, 4, req);
    req->window = w;
    req->valueMask = CWEventMask;
    OneDataCard32 (dpy, NEXTPTR(req,xChangeWindowAttributesReq), mask);
    UnlockDisplay(dpy);
    SyncHandle();
}

