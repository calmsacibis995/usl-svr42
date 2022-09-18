/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:GetWMCMapW.c	1.1"
/* $XConsortium: GetWMCMapW.c,v 1.5 91/02/01 16:33:05 gildea Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "Xlibint.h"
#include <X11/Xatom.h>
#include <stdio.h>

Status XGetWMColormapWindows (dpy, w, colormapWindows, countReturn)
    Display *dpy;
    Window w;
    Window **colormapWindows;
    int *countReturn;
{
    Atom *data = NULL;
    Atom actual_type;
    Atom prop;
    int actual_format;
    unsigned long leftover, nitems;

    prop =  XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
    if (prop == None) return False;

    /* get the property */
    if (XGetWindowProperty (dpy, w, prop,
    			    0L, 1000000L, False,
			    XA_WINDOW, &actual_type, &actual_format,
			    &nitems, &leftover, (unsigned char **) &data)
	!= Success)
      return False;

    if (actual_type != XA_WINDOW || actual_format != 32) {
	if (data) Xfree ((char *) data);
	return False;
    }

    *colormapWindows = (Window *) data;
    *countReturn = (int) nitems;
    return True;
}
