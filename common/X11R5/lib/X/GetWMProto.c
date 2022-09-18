/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:GetWMProto.c	1.1"
/* $XConsortium: GetWMProto.c,v 1.5 91/02/01 16:33:09 gildea Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "Xlibint.h"
#include <X11/Xatom.h>
#include <stdio.h>

Status XGetWMProtocols (dpy, w, protocols, countReturn)
    Display *dpy;
    Window w;
    Atom **protocols;
    int *countReturn;
{
    Atom *data = NULL;
    Atom actual_type;
    Atom prop;
    int actual_format;
    unsigned long leftover, nitems;

    prop =  XInternAtom(dpy, "WM_PROTOCOLS", False);
    if (prop == None) return False;

    /* get the property */
    if (XGetWindowProperty (dpy, w, prop,
    			    0L, 1000000L, False,
			    XA_ATOM, &actual_type, &actual_format,
			    &nitems, &leftover, (unsigned char **) &data)
	!= Success)
      return False;

    if (actual_type != XA_ATOM || actual_format != 32) {
	if (data) Xfree ((char *) data);
	return False;
    }

    *protocols = (Atom *) data;
    *countReturn = (int) nitems;
    return True;
}
