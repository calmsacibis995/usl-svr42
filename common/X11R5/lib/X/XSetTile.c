/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetTile.c	1.1"
/* $XConsortium: XSetTile.c,v 11.14 91/01/06 11:48:16 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetTile (dpy, gc, tile)
register Display *dpy;
GC gc;
Pixmap tile;
{
    LockDisplay(dpy);
    /* always update, since client may have changed pixmap contents */
    gc->values.tile = tile;
    gc->dirty |= GCTile;
    _XFlushGCCache(dpy, gc);
    UnlockDisplay(dpy);
    SyncHandle();
}
