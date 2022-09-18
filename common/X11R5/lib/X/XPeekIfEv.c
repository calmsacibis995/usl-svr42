/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XPeekIfEv.c	1.1"
/* $XConsortium: XPeekIfEv.c,v 11.13 91/01/06 11:47:15 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

/*
 * return the next event in the queue that satisfies the predicate.
 * BUT do not remove it from the queue.
 * If none found, flush, and then wait until one satisfies the predicate.
 */

XPeekIfEvent (dpy, event, predicate, arg)
	register Display *dpy;
	register XEvent *event;
	Bool (*predicate)(
#if NeedNestedPrototypes
			  Display*			/* display */,
			  XEvent*			/* event */,
			  char*				/* arg */
#endif
			  );
	char *arg;
{
	register _XQEvent *prev, *qelt;

	LockDisplay(dpy);
	prev = NULL;
	while (1) {
	    for (qelt = prev ? prev->next : dpy->head;
		 qelt;
		 prev = qelt, qelt = qelt->next) {
		if ((*predicate)(dpy, &qelt->event, arg)) {
		    *event = qelt->event;
		    UnlockDisplay(dpy);
		    return;
		}
	    }
	    _XReadEvents(dpy);
	}
}

