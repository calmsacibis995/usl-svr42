/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetIFocus.c	1.1"
/* $XConsortium: XGetIFocus.c,v 11.10 91/01/06 11:46:05 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

XGetInputFocus(dpy, focus, revert_to)
     register Display *dpy;
     Window *focus;
     int *revert_to;
{       
    xGetInputFocusReply rep;
    register xReq *req;
    LockDisplay(dpy);
    GetEmptyReq(GetInputFocus, req);
    (void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
    *focus = rep.focus;
    *revert_to = rep.revertTo;
    UnlockDisplay(dpy);
    SyncHandle();
}

