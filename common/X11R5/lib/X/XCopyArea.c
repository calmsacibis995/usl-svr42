/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XCopyArea.c	1.1"
/* $XConsortium: XCopyArea.c,v 11.8 91/01/06 11:44:46 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XCopyArea(dpy, src_drawable, dst_drawable, gc,
	  src_x, src_y, width, height,
	  dst_x, dst_y)
     register Display *dpy;
     Drawable src_drawable, dst_drawable;
     GC gc;
     int src_x, src_y;
     unsigned int width, height;
     int dst_x, dst_y;

{
    register xCopyAreaReq *req;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(CopyArea, req);
    req->srcDrawable = src_drawable;
    req->dstDrawable = dst_drawable;
    req->gc = gc->gid;
    req->srcX = src_x;
    req->srcY = src_y;
    req->dstX = dst_x;
    req->dstY = dst_y;
    req->width = width;
    req->height = height;
    UnlockDisplay(dpy);
    SyncHandle();
}

