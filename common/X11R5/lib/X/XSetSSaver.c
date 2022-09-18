/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetSSaver.c	1.1"
/* $XConsortium: XSetSSaver.c,v 11.7 91/01/06 11:48:10 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetScreenSaver(dpy, timeout, interval, prefer_blank, allow_exp)
    register Display *dpy;
    int timeout, interval, prefer_blank, allow_exp;

{
    register xSetScreenSaverReq *req;

    LockDisplay(dpy);
    GetReq(SetScreenSaver, req);
    req->timeout = timeout;
    req->interval = interval;
    req->preferBlank = prefer_blank;
    req->allowExpose = allow_exp;
    UnlockDisplay(dpy);
    SyncHandle();
}

