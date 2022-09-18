/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XStColor.c	1.1"
/* $XConsortium: XStColor.c,v 11.10 91/01/06 11:48:18 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XStoreColor(dpy, cmap, def)
register Display *dpy;
Colormap cmap;
XColor *def;
{
    xColorItem *citem;
    register xStoreColorsReq *req;
#ifdef MUSTCOPY
    xColorItem citemdata;
    long len = SIZEOF(xColorItem);

    citem = &citemdata;
#endif /* MUSTCOPY */

    LockDisplay(dpy);
    GetReqExtra(StoreColors, SIZEOF(xColorItem), req); /* assume size is 4*n */

    req->cmap = cmap;

#ifndef MUSTCOPY
    citem = (xColorItem *) NEXTPTR(req,xStoreColorsReq);
#endif /* not MUSTCOPY */

    citem->pixel = def->pixel;
    citem->red = def->red;
    citem->green = def->green;
    citem->blue = def->blue;
    citem->flags = def->flags; /* do_red, do_green, do_blue */

#ifdef MUSTCOPY
    dpy->bufptr -= SIZEOF(xColorItem);		/* adjust for GetReqExtra */
    Data (dpy, (char *) citem, len);
#endif /* MUSTCOPY */

    UnlockDisplay(dpy);
    SyncHandle();
}
