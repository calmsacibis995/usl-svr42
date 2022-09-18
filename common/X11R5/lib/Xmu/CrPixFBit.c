/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:CrPixFBit.c	1.2"
/*
 * $XConsortium: CrPixFBit.c,v 1.3 90/12/19 18:58:49 converse Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 *
 * This file contains miscellaneous utility routines and is not part of the
 * Xlib standard.
 *
 * Public entry points:
 *
 *     XmuCreatePixmapFromBitmap	make a pixmap from a bitmap
 */

#include <X11/Xos.h>
#include <X11/Xlib.h>

Pixmap XmuCreatePixmapFromBitmap (dpy, d, bitmap, width, height, depth,
				  fore, back)
    Display *dpy;			/* connection to X server */
    Drawable d;				/* drawable indicating screen */
    Pixmap bitmap;			/* single plane pixmap */
    unsigned int width, height;		/* dimensions of bitmap and pixmap */
    unsigned int depth;			/* depth of pixmap to create */
    unsigned long fore, back;		/* colors to use */
{
    Pixmap pixmap;

    pixmap = XCreatePixmap (dpy, d, width, height, depth);
    if (pixmap != None) {
	GC gc;
	XGCValues xgcv;

	xgcv.foreground = fore;
	xgcv.background = back;
	xgcv.graphics_exposures = False;

	gc = XCreateGC (dpy, d,
			(GCForeground | GCBackground | GCGraphicsExposures),
			&xgcv);
	if (gc) {
	    XCopyPlane (dpy, bitmap, pixmap, gc, 0, 0, width, height, 0, 0, 1);
	    XFreeGC (dpy, gc);
	} else {
	    XFreePixmap (dpy, pixmap);
	    pixmap = None;
	}
    }
    return pixmap;
}
