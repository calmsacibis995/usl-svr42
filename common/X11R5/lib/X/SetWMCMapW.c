/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:SetWMCMapW.c	1.1"
/* $XConsortium: SetWMCMapW.c,v 1.4 91/01/06 11:43:52 rws Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "Xlibint.h"
#include <X11/Xatom.h>

/* 
 * XSetWMProtocols sets the property 
 *	WM_COLORMAP_WINDOWS 	type: WINDOW	format:32
 */

Status XSetWMColormapWindows (dpy, w, windows, count)
    Display *dpy;
    Window w;
    Window *windows;
    int count;
{
    Atom prop;

    prop = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
    if (prop == None) return False;

    XChangeProperty (dpy, w, prop, XA_WINDOW, 32,
		     PropModeReplace, (unsigned char *) windows, count);
    return True;
}
