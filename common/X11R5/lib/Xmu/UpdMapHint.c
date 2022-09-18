/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:UpdMapHint.c	1.2"
/*
 * $XConsortium: UpdMapHint.c,v 1.1 89/09/22 12:07:37 jim Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

Bool XmuUpdateMapHints (dpy, w, hints)
    Display *dpy;
    Window w;
    XSizeHints *hints;
{
    static XSizeHints *shp = NULL;

    if (!hints) {				/* get them first */
	long supp;

	if (!shp) {
	    shp = XAllocSizeHints();
	    if (!shp) return False;
	}
	if (!XGetWMNormalHints (dpy, w, shp, &supp)) return False;
	hints = shp;
    }
    hints->flags &= ~(PPosition|PSize);
    hints->flags |= (USPosition|USSize);
    XSetWMNormalHints (dpy, w, hints);
    return True;
}
    
