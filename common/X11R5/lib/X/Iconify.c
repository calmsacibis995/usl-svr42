/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Iconify.c	1.1"
/* $XConsortium: Iconify.c,v 1.7 91/03/29 13:45:36 gildea Exp $ */

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
 * NormalState to IconicState.
 */
Status XIconifyWindow (dpy, w, screen)
    Display *dpy;
    Window w;
    int screen;
{
    XClientMessageEvent ev;
    Window root = RootWindow (dpy, screen);
    Atom prop;

    prop = XInternAtom (dpy, "WM_CHANGE_STATE", False);
    if (prop == None) return False;

    ev.type = ClientMessage;
    ev.window = w;
    ev.message_type = prop;
    ev.format = 32;
    ev.data.l[0] = IconicState;
    return (XSendEvent (dpy, root, False,
			SubstructureRedirectMask|SubstructureNotifyMask,
			(XEvent *)&ev));
}
