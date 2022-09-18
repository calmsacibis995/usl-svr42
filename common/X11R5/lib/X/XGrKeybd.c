/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGrKeybd.c	1.1"
/* $XConsortium: XGrKeybd.c,v 11.16 91/01/06 11:46:25 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"
int XGrabKeyboard (dpy, window, ownerEvents, pointerMode, keyboardMode, time)
    register Display *dpy;
    Window window;
    Bool ownerEvents;
    int pointerMode, keyboardMode;
    Time time;
{
        xGrabKeyboardReply rep;
	register xGrabKeyboardReq *req;
	register int status;
	LockDisplay(dpy);
        GetReq(GrabKeyboard, req);
	req->grabWindow = window;
	req->ownerEvents = ownerEvents;
	req->pointerMode = pointerMode;
	req->keyboardMode = keyboardMode;
	req->time = time;

       /* if we ever return, suppress the error */
	if (_XReply (dpy, (xReply *) &rep, 0, xTrue) == 0) 
		rep.status = GrabSuccess;
	status = rep.status;
	UnlockDisplay(dpy);
	SyncHandle();
	return (status);
}

