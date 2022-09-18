/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XQuColor.c	1.1"
/* $XConsortium: XQuColor.c,v 11.17 91/01/06 11:47:28 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

XQueryColor(dpy, cmap, def)
    register Display *dpy;
    Colormap cmap;
    XColor *def;	/* RETURN */
{
    xrgb color;
    xQueryColorsReply rep;
    register xQueryColorsReq *req;
    unsigned long val = def->pixel;	/* needed for macro below */

    LockDisplay(dpy);
    GetReqExtra(QueryColors, 4, req); /* a pixel (CARD32) is 4 bytes */
    req->cmap = cmap;

    OneDataCard32 (dpy, NEXTPTR(req,xQueryColorsReq), val);

    if (_XReply(dpy, (xReply *) &rep, 0, xFalse) != 0) {

	    _XRead(dpy, (char *)&color, (long) SIZEOF(xrgb));

	    def->red = color.red;
	    def->blue = color.blue;
	    def->green = color.green;
	    def->flags = DoRed | DoGreen | DoBlue;
        }
    UnlockDisplay(dpy);
    SyncHandle();
}
