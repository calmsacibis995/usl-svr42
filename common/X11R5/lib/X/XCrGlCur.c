/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XCrGlCur.c	1.1"
/* $XConsortium: XCrGlCur.c,v 11.8 91/01/06 11:44:56 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

Cursor XCreateGlyphCursor(dpy, source_font, mask_font,
		   source_char, mask_char,
		   foreground, background)
     register Display *dpy;
     Font source_font, mask_font;
     unsigned int source_char, mask_char;
     XColor *foreground, *background;

{       
    Cursor cid;
    register xCreateGlyphCursorReq *req;

    LockDisplay(dpy);
    GetReq(CreateGlyphCursor, req);
    cid = req->cid = XAllocID(dpy);
    req->source = source_font;
    req->mask = mask_font;
    req->sourceChar = source_char;
    req->maskChar = mask_char;
    req->foreRed = foreground->red;
    req->foreGreen = foreground->green;
    req->foreBlue = foreground->blue;
    req->backRed = background->red;
    req->backGreen = background->green;
    req->backBlue = background->blue;
    UnlockDisplay(dpy);
    SyncHandle();
    return (cid);
}

