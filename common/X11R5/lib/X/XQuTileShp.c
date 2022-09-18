/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XQuTileShp.c	1.1"
/* $XConsortium: XQuTileShp.c,v 11.11 91/01/06 11:47:39 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

Status XQueryBestTile(dpy, drawable, width, height, ret_width, ret_height)
     register Display *dpy;
     Drawable drawable;
     unsigned int width, height;
     unsigned int *ret_width, *ret_height;
{       
    xQueryBestSizeReply rep;
    register xQueryBestSizeReq *req;

    LockDisplay(dpy);
    GetReq(QueryBestSize, req);
    req->class = TileShape;
    req->drawable = drawable;
    req->width = width;
    req->height = height;
    if (_XReply (dpy, (xReply *)&rep, 0, xTrue) == 0) {
	UnlockDisplay(dpy);
	SyncHandle();
	return(0);
	}

    *ret_width = rep.width;
    *ret_height = rep.height;
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

