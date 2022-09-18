/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:ScrOfWin.c	1.2"
/*
 * $XConsortium: ScrOfWin.c,v 1.2 89/09/22 12:10:01 jim Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#include <stdio.h>
#include <X11/Xlib.h>


Screen *XmuScreenOfWindow (dpy, w)
    Display *dpy;
    Window w;
{
    register int i;
    Window root;
    int x, y;					/* dummy variables */
    unsigned int width, height, bw, depth;	/* dummy variables */

    if (!XGetGeometry (dpy, w, &root, &x, &y, &width, &height,
		       &bw, &depth)) {
	return NULL;
    }
    for (i = 0; i < ScreenCount (dpy); i++) {	/* find root from list */
	if (root == RootWindow (dpy, i)) {
	    return ScreenOfDisplay (dpy, i);
	}
    }
    return NULL;
}

