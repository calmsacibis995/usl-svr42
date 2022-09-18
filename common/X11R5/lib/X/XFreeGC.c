/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XFreeGC.c	1.2"

#include "Xlibint.h"

XFreeGC (dpy, gc)
    register Display *dpy;
    GC gc;
    {
    register xResourceReq *req;
    register _XExtension *ext;
    LockDisplay(dpy);
    ext = dpy->ext_procs;
    while (ext) {		/* call out to any extensions interested */
	if (ext->free_GC != NULL) (*ext->free_GC)(dpy, gc, &ext->codes);
	ext = ext->next;
	}    
    GetResReq (FreeGC, gc->gid, req);
    UnlockDisplay(dpy);
    SyncHandle();
    _XFreeExtData(gc->ext_data);
    Xfree ((char *) gc);
    }
    
