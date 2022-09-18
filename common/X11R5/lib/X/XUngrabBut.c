/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XUngrabBut.c	1.1"
/* $XConsortium: XUngrabBut.c,v 11.7 91/01/06 11:48:33 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XUngrabButton(dpy, button, modifiers, grab_window)
register Display *dpy;
unsigned int button; /* CARD8 */
unsigned int modifiers; /* CARD16 */
Window grab_window;
{
    register xUngrabButtonReq *req;

    LockDisplay(dpy);
    GetReq(UngrabButton, req);
    req->button = button;
    req->modifiers = modifiers;
    req->grabWindow = grab_window;
    UnlockDisplay(dpy);
    SyncHandle();
}
