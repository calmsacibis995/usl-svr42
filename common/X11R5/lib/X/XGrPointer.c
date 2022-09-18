/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGrPointer.c	1.1"
/* $XConsortium: XGrPointer.c,v 11.17 91/01/06 11:46:26 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

int XGrabPointer(dpy, grab_window, owner_events, event_mask, pointer_mode,
	     keyboard_mode, confine_to, curs, time)
register Display *dpy;
Window grab_window;
Bool owner_events;
unsigned int event_mask; /* CARD16 */
int pointer_mode, keyboard_mode;
Window confine_to;
Cursor curs;
Time time;
{
    xGrabPointerReply rep;
    register xGrabPointerReq *req;
    register int status;
    LockDisplay(dpy);
    GetReq(GrabPointer, req);
    req->grabWindow = grab_window;
    req->ownerEvents = owner_events;
    req->eventMask = event_mask;
    req->pointerMode = pointer_mode;
    req->keyboardMode = keyboard_mode;
    req->confineTo = confine_to;
    req->cursor = curs;
    req->time = time;
    
    /* if we ever return, suppress the error */
    if (_XReply (dpy, (xReply *) &rep, 0, xTrue) == 0)
	rep.status = GrabSuccess;
    status = rep.status;
    UnlockDisplay(dpy);
    SyncHandle();
    return (status);
}
