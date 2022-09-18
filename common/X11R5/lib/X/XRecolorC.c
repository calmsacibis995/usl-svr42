/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XRecolorC.c	1.1"
/* $XConsortium: XRecolorC.c,v 11.8 91/01/06 11:47:44 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XRecolorCursor(dpy, cursor, foreground, background)
     register Display *dpy;
     Cursor cursor;
     XColor *foreground, *background;
{       
    register xRecolorCursorReq *req;

    LockDisplay(dpy);
    GetReq(RecolorCursor, req);
    req->cursor = cursor;
    req->foreRed = foreground->red;
    req->foreGreen = foreground->green;
    req->foreBlue = foreground->blue;
    req->backRed = background->red;
    req->backGreen = background->green;
    req->backBlue = background->blue;
    UnlockDisplay(dpy);
    SyncHandle();
}

