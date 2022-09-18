/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XPutBEvent.c	1.1"
/* Copyright 	Massachusetts Institute of Technology  1986 */
/* $XConsortium: XPutBEvent.c,v 11.11 91/01/06 11:47:23 rws Exp $ */

/*
*/

/* XPutBackEvent puts an event back at the head of the queue. */
#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

XPutBackEvent (dpy, event)
	register Display *dpy;
	register XEvent *event;
	{
	register _XQEvent *qelt;

	LockDisplay(dpy);
	if (!_qfree) {
    	    if ((_qfree = (_XQEvent *) Xmalloc (sizeof (_XQEvent))) == NULL) {
		UnlockDisplay(dpy);
		return;
	    }
	    _qfree->next = NULL;
	}
	qelt = _qfree;
	_qfree = qelt->next;
	qelt->next = dpy->head;
	qelt->event = *event;
	dpy->head = qelt;
	if (dpy->tail == NULL)
	    dpy->tail = qelt;
	dpy->qlen++;
	UnlockDisplay(dpy);
	}
