/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XAllowEv.c	1.1"
/* $XConsortium: XAllowEv.c,v 1.13 91/01/06 11:44:05 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XAllowEvents(dpy, mode, time)
    register Display *dpy;
    int mode;
    Time time;

{
    register xAllowEventsReq *req;

    LockDisplay(dpy);
    GetReq(AllowEvents,req);
    req->mode = mode;
    req->time = time;
    UnlockDisplay(dpy);
    SyncHandle();
}



