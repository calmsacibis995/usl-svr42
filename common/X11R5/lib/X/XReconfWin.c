/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XReconfWin.c	1.1"
/* $XConsortium: XReconfWin.c,v 11.14 91/01/06 11:47:45 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

#define AllMaskBits (CWX|CWY|CWWidth|CWHeight|\
		     CWBorderWidth|CWSibling|CWStackMode)

XConfigureWindow(dpy, w, mask, changes)
    register Display *dpy;
    Window w;
    unsigned int mask;
    XWindowChanges *changes;
    {
    unsigned long values[7];
    register unsigned long *value = values;
    long nvalues;
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReq(ConfigureWindow, req);
    req->window = w;
    mask &= AllMaskBits;
    req->mask = mask;

    /* Warning!  This code assumes that "unsigned long" is 32-bits wide */

    if (mask & CWX)
	*value++ = changes->x;
	
    if (mask & CWY)
    	*value++ = changes->y;

    if (mask & CWWidth)
    	*value++ = changes->width;

    if (mask & CWHeight)
    	*value++ = changes->height;

    if (mask & CWBorderWidth)
    	*value++ = changes->border_width;

    if (mask & CWSibling)
	*value++ = changes->sibling;

    if (mask & CWStackMode)
        *value++ = changes->stack_mode;

    req->length += (nvalues = value - values);

    nvalues <<= 2;			/* watch out for macros... */
    Data32 (dpy, (long *) values, nvalues);
    UnlockDisplay(dpy);
    SyncHandle();

    }
