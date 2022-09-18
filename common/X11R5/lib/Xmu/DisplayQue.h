/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:DisplayQue.h	1.2"
/* $XConsortium: DisplayQue.h,v 1.5 91/07/22 23:45:45 converse Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 */

#ifndef _XMU_DISPLAYQUE_H_
#define _XMU_DISPLAYQUE_H_

#include <X11/Xmu/CloseHook.h>
#include <X11/Xfuncproto.h>

/*
 *			      Public Entry Points
 * 
 * 
 * XmuDisplayQueue *XmuDQCreate (closefunc, freefunc, data)
 *     XmuCloseDisplayQueueProc closefunc;
 *     XmuFreeDisplayQueueProc freefunc;
 *     caddr_t data;
 * 
 *         Creates and returns a queue into which displays may be placed.  When
 *         the display is closed, the closefunc (if non-NULL) is upcalled with
 *         as follows:
 *
 *                 (*closefunc) (queue, entry)
 *
 *         The freeproc, if non-NULL, is called whenever the last display is
 *         closed, notifying the creator that display queue may be released
 *         using XmuDQDestroy.
 *
 *
 * Bool XmuDQDestroy (q, docallbacks)
 *     XmuDisplayQueue *q;
 *     Bool docallbacks;
 * 
 *         Releases all memory for the indicated display queue.  If docallbacks
 *         is true, then the closefunc (if non-NULL) is called for each 
 *         display.
 * 
 * 
 * XmuDisplayQueueEntry *XmuDQLookupDisplay (q, dpy)
 *     XmuDisplayQueue *q;
 *     Display *dpy;
 *
 *         Returns the queue entry for the specified display or NULL if the
 *         display is not in the queue.
 *
 * 
 * XmuDisplayQueueEntry *XmuDQAddDisplay (q, dpy, data)
 *     XmuDisplayQueue *q;
 *     Display *dpy;
 *     caddr_t data;
 *
 *         Adds the indicated display to the end of the queue or NULL if it
 *         is unable to allocate memory.  The data field may be used by the
 *         caller to attach arbitrary data to this display in this queue.  The
 *         caller should use XmuDQLookupDisplay to make sure that the display
 *         hasn't already been added.
 * 
 * 
 * Bool XmuDQRemoveDisplay (q, dpy)
 *     XmuDisplayQueue *q;
 *     Display *dpy;
 *
 *         Removes the specified display from the given queue.  If the 
 *         indicated display is not found on this queue, False is returned,
 *         otherwise True is returned.
 */

typedef struct _XmuDisplayQueue XmuDisplayQueue;
typedef struct _XmuDisplayQueueEntry XmuDisplayQueueEntry;

typedef int (*XmuCloseDisplayQueueProc)(
#if NeedFunctionPrototypes
    XmuDisplayQueue*		/* queue */,
    XmuDisplayQueueEntry*	/* entry */
#endif
);

typedef int (*XmuFreeDisplayQueueProc)(
#if NeedFunctionPrototypes
    XmuDisplayQueue*		/* queue */
#endif
);

struct _XmuDisplayQueueEntry {
    struct _XmuDisplayQueueEntry *prev, *next;
    Display *display;
    CloseHook closehook;
    caddr_t data;
};

struct _XmuDisplayQueue {
    int nentries;
    XmuDisplayQueueEntry *head, *tail;
    XmuCloseDisplayQueueProc closefunc;
    XmuFreeDisplayQueueProc freefunc;
    caddr_t data;
};

_XFUNCPROTOBEGIN

extern XmuDisplayQueue *XmuDQCreate(
#if NeedFunctionPrototypes
    XmuCloseDisplayQueueProc	/* closefunc */,
    XmuFreeDisplayQueueProc	/* freefunc */,
    caddr_t	/* data */
#endif
);

extern Bool XmuDQDestroy(
#if NeedFunctionPrototypes
    XmuDisplayQueue*	/* q */,
    Bool		/* docallbacks */
#endif
);

extern XmuDisplayQueueEntry *XmuDQLookupDisplay(
#if NeedFunctionPrototypes
    XmuDisplayQueue*	/* q */,
    Display*		/* dpy */
#endif
);

extern XmuDisplayQueueEntry *XmuDQAddDisplay(
#if NeedFunctionPrototypes
    XmuDisplayQueue*	/* q */,
    Display*		/* dpy */,
    caddr_t		/* data */
#endif
);

extern Bool XmuDQRemoveDisplay(
#if NeedFunctionPrototypes
    XmuDisplayQueue*	/* q */,
    Display*		/* dpy */
#endif
);

_XFUNCPROTOEND

#define XmuDQNDisplays(q) ((q)->nentries)

#endif /* _XMU_DISPLAYQUE_H_ */
