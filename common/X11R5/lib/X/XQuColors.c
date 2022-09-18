/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XQuColors.c	1.1"
/* $XConsortium: XQuColors.c,v 11.20 91/01/06 11:47:29 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

XQueryColors(dpy, cmap, defs, ncolors)
    register Display *dpy;
    Colormap cmap;
    XColor *defs; 		/* RETURN */
    int ncolors;
{
    register int i;
    xrgb *color;
    xQueryColorsReply rep;
    long nbytes;
    register xQueryColorsReq *req;

    LockDisplay(dpy);
    GetReq(QueryColors, req);

    req->cmap = cmap;
    req->length += ncolors; /* each pixel is a CARD32 */

    for (i = 0; i < ncolors; i++)
      Data32 (dpy, (long *)&defs[i].pixel, 4L);
       /* XXX this isn't very efficient */

    if (_XReply(dpy, (xReply *) &rep, 0, xFalse) != 0) {
	if (color = (xrgb *)
	    Xmalloc((unsigned) (nbytes = (long) ncolors * SIZEOF(xrgb)))) {

	    _XRead(dpy, (char *) color, nbytes);

	    for (i = 0; i < ncolors; i++) {
		register XColor *def = &defs[i];
		register xrgb *rgb = &color[i];
		def->red = rgb->red;
		def->green = rgb->green;
		def->blue = rgb->blue;
		def->flags = DoRed | DoGreen | DoBlue;
	    }
	    Xfree((char *)color);
	}
	else _XEatData(dpy, (unsigned long) nbytes);
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

