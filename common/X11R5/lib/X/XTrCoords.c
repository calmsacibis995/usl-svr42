/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XTrCoords.c	1.1"
/* $XConsortium: XTrCoords.c,v 11.14 91/01/06 11:48:31 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

Bool XTranslateCoordinates(dpy, src_win, dest_win, src_x, src_y, 
		      dst_x, dst_y, child)
     register Display *dpy;
     Window src_win, dest_win;
     int src_x, src_y;
     int *dst_x, *dst_y;
     Window *child;
{       
    register xTranslateCoordsReq *req;
    xTranslateCoordsReply rep;

    LockDisplay(dpy);
    GetReq(TranslateCoords, req);
    req->srcWid = src_win;
    req->dstWid = dest_win;
    req->srcX = src_x;
    req->srcY = src_y;
    if (_XReply (dpy, (xReply *)&rep, 0, xTrue) == 0) {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return(False);
	}
	
    *child = rep.child;
    *dst_x = cvtINT16toInt (rep.dstX);
    *dst_y = cvtINT16toInt (rep.dstY);
    UnlockDisplay(dpy);
    SyncHandle();
    return ((int)rep.sameScreen);
}

