/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XStColors.c	1.1"
/* $XConsortium: XStColors.c,v 11.14 91/01/06 11:48:19 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XStoreColors(dpy, cmap, defs, ncolors)
register Display *dpy;
Colormap cmap;
XColor *defs;
int ncolors;
{
    register int i;
    xColorItem citem;
    register xStoreColorsReq *req;

    LockDisplay(dpy);    
    GetReq(StoreColors, req);

    req->cmap = cmap;

    req->length += (ncolors * SIZEOF(xColorItem)) >> 2; /* assume size is 4*n */

    for (i = 0; i < ncolors; i++) {
	citem.pixel = defs[i].pixel;
	citem.red = defs[i].red;
	citem.green = defs[i].green;
	citem.blue = defs[i].blue;
	citem.flags = defs[i].flags;

	/* note that xColorItem doesn't contain all 16-bit quantities, so
	   we can't use Data16 */
	Data(dpy, (char *)&citem, (long) SIZEOF(xColorItem)); 
			/* assume size is 4*n */
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
