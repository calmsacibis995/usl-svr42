/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGCMisc.c	1.1"
/* $XConsortium: XGCMisc.c,v 11.4 91/01/06 11:45:50 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetArcMode (dpy, gc, arc_mode)
register Display *dpy;
register GC gc;
int arc_mode;
{
    LockDisplay(dpy);
    if (gc->values.arc_mode != arc_mode) {
	gc->values.arc_mode = arc_mode;
	gc->dirty |= GCArcMode;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

XSetFillRule (dpy, gc, fill_rule)
register Display *dpy;
register GC gc;
int fill_rule;
{
    LockDisplay(dpy);
    if (gc->values.fill_rule != fill_rule) {
	gc->values.fill_rule = fill_rule;
	gc->dirty |= GCFillRule;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

XSetFillStyle (dpy, gc, fill_style)
register Display *dpy;
register GC gc;
int fill_style;
{
    LockDisplay(dpy);
    if (gc->values.fill_style != fill_style) {
	gc->values.fill_style = fill_style;
	gc->dirty |= GCFillStyle;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

XSetGraphicsExposures (dpy, gc, graphics_exposures)
register Display *dpy;
register GC gc;
Bool graphics_exposures;
{
    LockDisplay(dpy);
    if (gc->values.graphics_exposures != graphics_exposures) {
	gc->values.graphics_exposures = graphics_exposures;
	gc->dirty |= GCGraphicsExposures;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

XSetSubwindowMode (dpy, gc, subwindow_mode)
register Display *dpy;
register GC gc;
int subwindow_mode;
{
    LockDisplay(dpy);
    if (gc->values.subwindow_mode != subwindow_mode) {
	gc->values.subwindow_mode = subwindow_mode;
	gc->dirty |= GCSubwindowMode;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
