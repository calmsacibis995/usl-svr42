/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetClMask.c	1.1"
/* $XConsortium: XSetClMask.c,v 11.9 91/01/06 11:47:54 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetClipMask (dpy, gc, mask)
register Display *dpy;
GC gc;
Pixmap mask;
{
    LockDisplay(dpy);
    /* always update, since client may have changed pixmap contents */
    gc->values.clip_mask = mask;
    gc->dirty |= GCClipMask;
    gc->rects = 0;
    _XFlushGCCache(dpy, gc);
    UnlockDisplay(dpy);
    SyncHandle();
}
