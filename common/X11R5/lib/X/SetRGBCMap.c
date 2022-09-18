/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:SetRGBCMap.c	1.1"
/* Copyright 1989 Massachusetts Institute of Technolgoy */
/* $XConsortium: SetRGBCMap.c,v 1.4 91/01/08 14:40:02 gildea Exp $ */

/*
*/

#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include "Xatomtype.h"
#include <X11/Xatom.h>

void XSetRGBColormaps (dpy, w, cmaps, count, property)
    Display *dpy;
    Window w;
    XStandardColormap *cmaps;
    int count;
    Atom property;			/* XA_RGB_BEST_MAP, etc. */
{
    register int i;			/* iterator variable */
    register xPropStandardColormap *map;  /* tmp variable, data in prop */
    register XStandardColormap *cmap;	/* tmp variable, user data */
    xPropStandardColormap *data, tmpdata;  /* scratch data */
    int mode = PropModeReplace;		/* for partial writes */
    Bool alloced_scratch_space;		/* do we need to free? */
	

    if (count < 1) return;

    /*
     * if doing more than one, allocate scratch space for it
     */
    if ((count > 1) && ((data = ((xPropStandardColormap *)
				 Xmalloc(count*sizeof(xPropStandardColormap))))
			 != NULL)) {
	alloced_scratch_space = True;
    } else {
	data = &tmpdata;
	alloced_scratch_space = False;
    }


    /*
     * Do the iteration.  If using temp space put out each part of the prop;
     * otherwise, wait until the end and blast it all at once.
     */
    for (i = count, map = data, cmap = cmaps; i > 0; i--, cmap++) {
	map->colormap	= cmap->colormap;
	map->red_max	= cmap->red_max;
	map->red_mult	= cmap->red_mult;
	map->green_max	= cmap->green_max;
	map->green_mult	= cmap->green_mult;
	map->blue_max	= cmap->blue_max;
	map->blue_mult	= cmap->blue_mult;
	map->base_pixel	= cmap->base_pixel;
	map->visualid	= cmap->visualid;
	map->killid	= cmap->killid;

	if (alloced_scratch_space) {
	    map++;
	} else {
	    XChangeProperty (dpy, w, property, XA_RGB_COLOR_MAP, 32, mode,
			     (unsigned char *) data,
			     NumPropStandardColormapElements);
	    mode = PropModeAppend;
	}
    }

    if (alloced_scratch_space) {
	XChangeProperty (dpy, w, property, XA_RGB_COLOR_MAP, 32,
			 PropModeReplace, (unsigned char *) data,
			 (int) (count * NumPropStandardColormapElements));
	Xfree ((char *) data);
    }
}
