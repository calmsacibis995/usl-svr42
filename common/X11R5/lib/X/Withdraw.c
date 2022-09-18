/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Withdraw.c	1.1"
/* $XConsortium: Withdraw.c,v 1.5 91/02/01 16:33:33 gildea Exp $ */

/***********************************************************
Copyright 1988 by Wyse Technology, Inc., San Jose, Ca.,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#define NEED_EVENTS
#include <X11/Xlibint.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <stdio.h>

/*
 * This function instructs the window manager to change this window from
 * NormalState or IconicState to Withdrawn.
 */
Status XWithdrawWindow (dpy, w, screen)
    Display *dpy;
    Window w;
    int screen;
{
    XUnmapEvent ev;
    Window root = RootWindow (dpy, screen);

    XUnmapWindow (dpy, w);

    ev.type = UnmapNotify;
    ev.event = root;
    ev.window = w;
    ev.from_configure = False;
    return (XSendEvent (dpy, root, False,
			SubstructureRedirectMask|SubstructureNotifyMask,
			(XEvent *)&ev));
}
