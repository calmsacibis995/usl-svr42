/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XScrResStr.c	1.1"
/* $XConsortium: XScrResStr.c,v 1.2 91/02/04 09:30:59 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1991	*/

/*
*/

#include "Xlibint.h"
#include <X11/Xatom.h>

char *XScreenResourceString(screen)
	Screen *screen;
{
    Atom prop_name;
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long leftover;
    char *val = NULL;

    prop_name = XInternAtom(screen->display, "SCREEN_RESOURCES", True);
    if (prop_name &&
	XGetWindowProperty(screen->display, screen->root, prop_name,
			   0L, 100000000L, False,
			   XA_STRING, &actual_type, &actual_format,
			   &nitems, &leftover,
			   (unsigned char **) &val) == Success) {
	if ((actual_type == XA_STRING) && (actual_format == 8))
	    return val;
	if (val)
	    Xfree(val);
    }
    return (char *)NULL;
}
