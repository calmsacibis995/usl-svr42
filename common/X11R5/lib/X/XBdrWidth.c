/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XBdrWidth.c	1.1"
/* $XConsortium: XBdrWidth.c,v 11.8 91/01/06 11:44:11 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1987 */

/*
*/

#include "Xlibint.h"

XSetWindowBorderWidth(dpy, w, width)
Display *dpy;
Window w;
unsigned int width;
{
    unsigned long lwidth = width;	/* must be CARD32 */

    register xConfigureWindowReq *req;
    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 4, req);
    req->window = w;
    req->mask = CWBorderWidth;
    OneDataCard32 (dpy, NEXTPTR(req,xConfigureWindowReq), lwidth);
    UnlockDisplay(dpy);
    SyncHandle();
}

