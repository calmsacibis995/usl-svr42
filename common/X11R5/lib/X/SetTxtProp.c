/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:SetTxtProp.c	1.1"
/* $XConsortium: SetTxtProp.c,v 1.5 91/02/01 16:33:29 gildea Exp $ */
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

void XSetTextProperty (dpy, w, tp, property)
    Display *dpy;
    Window w;
    Atom property;
    XTextProperty *tp;
{
    XChangeProperty (dpy, w, property, tp->encoding, tp->format,
		     PropModeReplace, tp->value, tp->nitems);
}

void XSetWMName (dpy, w, tp)
    Display *dpy;
    Window w;
    XTextProperty *tp;
{
    XSetTextProperty (dpy, w, tp, XA_WM_NAME);
}

void XSetWMIconName (dpy, w, tp)
    Display *dpy;
    Window w;
    XTextProperty *tp;
{
    XSetTextProperty (dpy, w, tp, XA_WM_ICON_NAME);
}

void XSetWMClientMachine (dpy, w, tp)
    Display *dpy;
    Window w;
    XTextProperty *tp;
{
    XSetTextProperty (dpy, w, tp, XA_WM_CLIENT_MACHINE);
}

