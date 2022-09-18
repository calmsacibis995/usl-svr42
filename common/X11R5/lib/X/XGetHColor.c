/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetHColor.c	1.1"
/* $XConsortium: XGetHColor.c,v 11.11 91/01/06 11:45:58 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

Status XAllocColor(dpy, cmap, def)
register Display *dpy;
Colormap cmap;
XColor *def;
{
    Status status;
    xAllocColorReply rep;
    register xAllocColorReq *req;
    LockDisplay(dpy);
    GetReq(AllocColor, req);

    req->cmap = cmap;
    req->red = def->red;
    req->green = def->green;
    req->blue = def->blue;

    status = _XReply(dpy, (xReply *) &rep, 0, xTrue);
    if (status) {
      def->pixel = rep.pixel;
      def->red = rep.red;
      def->green = rep.green;
      def->blue = rep.blue;
      }
    UnlockDisplay(dpy);
    SyncHandle();
    return(status);
}
