/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:PixFormats.c	1.1"
/* $XConsortium: PixFormats.c,v 1.5 91/02/01 16:33:23 gildea Exp $ */
/* Copyright 1989 Massachusetts Institute of Technology */

/*
*/

#include "Xlibint.h"
#include <stdio.h>

/*
 * XListPixmapFormats - return info from connection setup
 */

XPixmapFormatValues *XListPixmapFormats (dpy, count)
    Display *dpy;
    int *count;	/* RETURN */
{
    XPixmapFormatValues *formats = (XPixmapFormatValues *)
	Xmalloc((unsigned) (dpy->nformats * sizeof (XPixmapFormatValues)));

    if (formats) {
	register int i;
	register XPixmapFormatValues *f;
	register ScreenFormat *sf;

	/*
	 * copy data from internal Xlib data structure in display
	 */
	for (i = dpy->nformats, f = formats, sf = dpy->pixmap_format; i > 0;
	     i--, f++, sf++) {
	    f->depth = sf->depth;
	    f->bits_per_pixel = sf->bits_per_pixel;
	    f->scanline_pad = sf->scanline_pad;
	}

	*count = dpy->nformats;
    }
    return formats;
}
