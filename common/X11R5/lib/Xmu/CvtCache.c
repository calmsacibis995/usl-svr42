/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:CvtCache.c	1.3"
/*
 * $XConsortium: CvtCache.c,v 1.6 90/12/19 18:21:33 converse Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 * 
 * Author:  Jim Fulton, MIT X Consortium
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xmu/CvtCache.h>

#ifndef MEMUTIL
extern char *malloc();
#endif /* MEMUTIL */

static XmuDisplayQueue *dq = NULL;
static int _CloseDisplay(), _FreeCCDQ();



/*
 * internal utility callbacks
 */

static int _FreeCCDQ (q)
    XmuDisplayQueue *q;
{
    XmuDQDestroy (dq, False);
    dq = NULL;
}


static int _CloseDisplay (q, e)
    XmuDisplayQueue *q;
    XmuDisplayQueueEntry *e;
{
    XmuCvtCache *c;
    extern void _XmuStringToBitmapFreeCache();

    if (e && (c = (XmuCvtCache *)(e->data))) {
	_XmuStringToBitmapFreeCache (c);
	/* insert calls to free any cached memory */

    }
    return 0;
}

static void _InitializeCvtCache (c)
    register XmuCvtCache *c;
{
    extern void _XmuStringToBitmapInitCache();

    _XmuStringToBitmapInitCache (c);
    /* insert calls to init any cached memory */
}


/*
 * XmuCCLookupDisplay - return the cache entry for the indicated display;
 * initialize the cache if necessary
 */
XmuCvtCache *_XmuCCLookupDisplay (dpy)
    Display *dpy;
{
    XmuDisplayQueueEntry *e;

    /*
     * If no displays have been added before this, create the display queue.
     */
    if (!dq) {
	dq = XmuDQCreate (_CloseDisplay, _FreeCCDQ, NULL);
	if (!dq) return NULL;
    }
    
    /*
     * See if the display is already there
     */
    e = XmuDQLookupDisplay (dq, dpy);	/* see if it's there */
    if (!e) {				/* else create it */
	XmuCvtCache *c = (XmuCvtCache *) malloc (sizeof (XmuCvtCache));
	if (!c) return NULL;

	/*
	 * Add the display to the queue
	 */
	e = XmuDQAddDisplay (dq, dpy, (caddr_t) c);
	if (!e) {
	    free ((char *) c);
	    return NULL;
	}

	/*
	 * initialize fields in cache
	 */
	_InitializeCvtCache (c);
    }

    /*
     * got it
     */
    return (XmuCvtCache *)(e->data);
}


