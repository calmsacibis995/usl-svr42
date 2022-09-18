/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:SetWMProto.c	1.1"
/* $XConsortium: SetWMProto.c,v 1.4 91/01/06 11:43:54 rws Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "Xlibint.h"
#include <X11/Xatom.h>

/* 
 * XSetWMProtocols sets the property 
 *	WM_PROTOCOLS 	type: ATOM	format: 32
 */

Status XSetWMProtocols (dpy, w, protocols, count)
    Display *dpy;
    Window w;
    Atom *protocols;
    int count;
{
    Atom prop;

    prop = XInternAtom (dpy, "WM_PROTOCOLS", False);
    if (prop == None) return False;

    XChangeProperty (dpy, w, prop, XA_ATOM, 32,
		     PropModeReplace, (unsigned char *) protocols, count);
    return True;
}
