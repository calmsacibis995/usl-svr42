/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGrKey.c	1.1"
/* $XConsortium: XGrKey.c,v 11.8 91/01/06 11:46:23 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XGrabKey(dpy, key, modifiers, grab_window, owner_events, 
	 pointer_mode, keyboard_mode)
    register Display *dpy;
    int key;
    unsigned int modifiers;
    Window grab_window;
    Bool owner_events;
    int pointer_mode, keyboard_mode;

{
    register xGrabKeyReq *req;
    LockDisplay(dpy);
    GetReq(GrabKey, req);
    req->ownerEvents = owner_events;
    req->grabWindow = grab_window;
    req->modifiers = modifiers;
    req->key = key;
    req->pointerMode = pointer_mode;
    req->keyboardMode = keyboard_mode;
    UnlockDisplay(dpy);
    SyncHandle();
}



