/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:GetTxtProp.c	1.1"
/* $XConsortium: GetTxtProp.c,v 1.6 91/02/01 16:33:02 gildea Exp $ */
/***********************************************************
Copyright 1988 by Wyse Technology, Inc., San Jose, Ca.,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include <X11/Xlibint.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>

Status XGetTextProperty (display, window, tp, property)
    Display *display;
    Window window;
    XTextProperty *tp;
    Atom property;
{
    Atom actual_type;
    int actual_format = 0;
    unsigned long nitems = 0L, leftover = 0L;
    unsigned char *prop = NULL;

    if (XGetWindowProperty (display, window, property, 0L, 1000000L, False,
			    AnyPropertyType, &actual_type, &actual_format,
			    &nitems, &leftover, &prop) == Success &&
	actual_type != None) {
	/* okay, fill it in */
	tp->value = prop;
	tp->encoding = actual_type;
	tp->format = actual_format;
	tp->nitems = nitems;
	return True;
    }

    tp->value = NULL;
    tp->encoding = None;
    tp->format = 0;
    tp->nitems = 0;
    return False;
}

Status XGetWMName (dpy, w, tp)
    Display *dpy;
    Window w;
    XTextProperty *tp;
{
    return (XGetTextProperty (dpy, w, tp, XA_WM_NAME));
}

Status XGetWMIconName (dpy, w, tp)
    Display *dpy;
    Window w;
    XTextProperty *tp;
{
    return (XGetTextProperty (dpy, w, tp, XA_WM_ICON_NAME));
}

Status XGetWMClientMachine (dpy, w, tp)
    Display *dpy;
    Window w;
    XTextProperty *tp;
{
    return (XGetTextProperty (dpy, w, tp, XA_WM_CLIENT_MACHINE));
}

