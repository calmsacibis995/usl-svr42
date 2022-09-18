/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetStCmap.c	1.1"
/* $XConsortium: XSetStCmap.c,v 1.7 91/01/08 14:41:51 gildea Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include "Xatomtype.h"
#include <X11/Xatom.h>

/*
 * 				    WARNING
 * 
 * This is a pre-ICCCM routine.  It must not reference any of the new fields
 * in the XStandardColormap structure.
 */

void XSetStandardColormap(dpy, w, cmap, property)
    Display *dpy;
    Window w;
    XStandardColormap *cmap;
    Atom property;		/* XA_RGB_BEST_MAP, etc. */
{
    Screen *sp;
    XStandardColormap stdcmap;

    sp = _XScreenOfWindow (dpy, w);
    if (!sp) {
	/* already caught the XGetGeometry error in _XScreenOfWindow */
	return;
    }

    stdcmap.colormap	= cmap->colormap;
    stdcmap.red_max	= cmap->red_max;
    stdcmap.red_mult	= cmap->red_mult;
    stdcmap.green_max	= cmap->green_max;
    stdcmap.green_mult  = cmap->green_mult;
    stdcmap.blue_max	= cmap->blue_max;
    stdcmap.blue_mult	= cmap->blue_mult;
    stdcmap.base_pixel	= cmap->base_pixel;
    stdcmap.visualid	= sp->root_visual->visualid;
    stdcmap.killid	= None;		/* don't know how to kill this one */

    XSetRGBColormaps (dpy, w, &stdcmap, 1, property);
    return;
}
