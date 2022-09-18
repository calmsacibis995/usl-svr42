/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XUngrabKey.c	1.1"
/* $XConsortium: XUngrabKey.c,v 11.7 91/01/06 11:48:36 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XUngrabKey(dpy, key, modifiers, grab_window)
     register Display *dpy;
     int key;
     unsigned int modifiers;
     Window grab_window;

{
    register xUngrabKeyReq *req;

    LockDisplay(dpy);
    GetReq(UngrabKey, req);
    req->grabWindow = grab_window;
    req->modifiers = modifiers;
    req->key = key;
    UnlockDisplay(dpy);
    SyncHandle();
}



