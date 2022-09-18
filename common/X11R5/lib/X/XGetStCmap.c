/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetStCmap.c	1.1"
/* $XConsortium: XGetStCmap.c,v 1.10 91/01/08 14:41:00 gildea Exp $ */

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

Status XGetStandardColormap (dpy, w, cmap, property)
    Display *dpy;
    Window w;
    XStandardColormap *cmap;
    Atom property;		/* XA_RGB_BEST_MAP, etc. */
{
    Status stat;			/* return value */
    XStandardColormap *stdcmaps;	/* will get malloced value */
    int nstdcmaps;			/* count of above */

    stat = XGetRGBColormaps (dpy, w, &stdcmaps, &nstdcmaps, property);
    if (stat) {
	XStandardColormap *use;

	if (nstdcmaps > 1) {
	    VisualID vid;
	    Screen *sp = _XScreenOfWindow (dpy, w);
	    int i;

	    if (!sp) {
		if (stdcmaps) Xfree ((char *) stdcmaps);
		return False;
	    }
	    vid = sp->root_visual->visualid;

	    for (i = 0; i < nstdcmaps; i++) {
		if (stdcmaps[i].visualid == vid) break;
	    }

	    if (i == nstdcmaps) {	/* not found */
		Xfree ((char *) stdcmaps);
		return False;
	    }
	    use = &stdcmaps[i];
	} else {
	    use = stdcmaps;
	}
	    
	/*
	 * assign only those fields which were in the pre-ICCCM version
	 */
	cmap->colormap	 = use->colormap;
	cmap->red_max	 = use->red_max;
	cmap->red_mult	 = use->red_mult;
	cmap->green_max	 = use->green_max;
	cmap->green_mult = use->green_mult;
	cmap->blue_max	 = use->blue_max;
	cmap->blue_mult	 = use->blue_mult;
	cmap->base_pixel = use->base_pixel;

	Xfree ((char *) stdcmaps);	/* don't need alloced memory */
    }
    return stat;
}
