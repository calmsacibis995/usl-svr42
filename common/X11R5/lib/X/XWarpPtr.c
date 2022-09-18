/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XWarpPtr.c	1.1"
/* $XConsortium: XWarpPtr.c,v 11.7 91/01/06 11:48:47 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XWarpPointer(dpy, src_win, dest_win, src_x, src_y, src_width, src_height,
	     dest_x, dest_y)
     register Display *dpy;
     Window src_win, dest_win;
     int src_x, src_y;
     unsigned int src_width, src_height;
     int dest_x, dest_y;
{       
    register xWarpPointerReq *req;

    LockDisplay(dpy);
    GetReq(WarpPointer, req);
    req->srcWid = src_win;
    req->dstWid = dest_win;
    req->srcX = src_x;
    req->srcY = src_y;
    req->srcWidth = src_width;
    req->srcHeight = src_height;
    req->dstX = dest_x;
    req->dstY = dest_y;
    UnlockDisplay(dpy);
    SyncHandle();
}

