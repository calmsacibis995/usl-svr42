/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetGeom.c	1.1"
/* $XConsortium: XGetGeom.c,v 11.16 91/01/06 11:45:57 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

Status XGetGeometry (dpy, d, root, x, y, width, height, borderWidth, depth)
    register Display *dpy;
    Drawable d;
    Window *root; /* RETURN */
    int *x, *y;  /* RETURN */
    unsigned int *width, *height, *borderWidth, *depth;  /* RETURN */
{
    xGetGeometryReply rep;
    register xResourceReq *req;
    LockDisplay(dpy);
    GetResReq(GetGeometry, d, req);
    if (!_XReply (dpy, (xReply *)&rep, 0, xTrue)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return (0);
	}
    *root = rep.root;
    *x = cvtINT16toInt (rep.x);
    *y = cvtINT16toInt (rep.y);
    *width = rep.width;
    *height = rep.height;
    *borderWidth = rep.borderWidth;
    *depth = rep.depth;
    UnlockDisplay(dpy);
    SyncHandle();
    return (1);
}

