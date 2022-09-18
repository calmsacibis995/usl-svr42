/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSync.c	1.1"
/* $XConsortium: XSync.c,v 11.15 91/01/06 11:48:24 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

/* Synchronize with errors and events, optionally discarding pending events */

XSync (dpy, discard)
    register Display *dpy;
    Bool discard;
{
    xGetInputFocusReply rep;
    register xReq *req;

    LockDisplay(dpy);
    GetEmptyReq(GetInputFocus, req);
    (void) _XReply (dpy, (xReply *)&rep, 0, xTrue);

    if (discard && dpy->head) {
       ((_XQEvent *)dpy->tail)->next = _qfree;
       _qfree = (_XQEvent *)dpy->head;
       dpy->head = dpy->tail = NULL;
       dpy->qlen = 0;
    }
    UnlockDisplay(dpy);
}

