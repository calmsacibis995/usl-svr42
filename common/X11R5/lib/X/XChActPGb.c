/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XChActPGb.c	1.1"
/* $XConsortium: XChActPGb.c,v 11.8 91/01/06 11:44:16 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XChangeActivePointerGrab(dpy, event_mask, curs, time)
register Display *dpy;
unsigned int event_mask; /* CARD16 */
Cursor curs;
Time time;
{
    register xChangeActivePointerGrabReq *req;

    LockDisplay(dpy);
    GetReq(ChangeActivePointerGrab, req);
    req->eventMask = event_mask;
    req->cursor = curs;
    req->time = time;
    UnlockDisplay(dpy);
    SyncHandle();
}
