/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5x11perf:do_valgc.c	1.1"
/*****************************************************************************
Copyright 1988, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved



******************************************************************************/

#ifndef VMS
#include <X11/Xatom.h>
#else
#include <decw$include/Xatom.h>
#endif
#include "x11perf.h"

static Window win[2];

int InitGC(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    win[0] = XCreateSimpleWindow(
	xp->d, xp->w, 10, 10, 10, 10, 1, xp->foreground, xp->background);
    win[1] = XCreateSimpleWindow(
	xp->d, xp->w, 30, 30, 10, 10, 1, xp->foreground, xp->background);
    XMapSubwindows(xp->d, xp->w);
    return reps;
}

void DoChangeGC(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int		i;
    XGCValues   gcv;

    for (i = 0; i != reps; i++) {
        gcv.foreground = xp->foreground;
        XChangeGC(xp->d, xp->fggc, GCForeground , &gcv);
        XDrawPoint(xp->d, win[0], xp->fggc, 5, 5);       

        gcv.foreground = xp->background;
        XChangeGC(xp->d, xp->fggc, GCForeground , &gcv);
        XDrawPoint(xp->d, win[1], xp->fggc, 5, 5);       

        gcv.foreground = xp->background;
        XChangeGC(xp->d, xp->fggc, GCForeground , &gcv);
        XDrawPoint(xp->d, win[0], xp->fggc, 5, 5);       

        gcv.foreground = xp->foreground;
        XChangeGC(xp->d, xp->fggc, GCForeground , &gcv);
        XDrawPoint(xp->d, win[1], xp->fggc, 5, 5);       
    }
}

void EndGC(xp, p)
    XParms  xp;
    Parms   p;
{
    XDestroyWindow(xp->d, win[0]);
    XDestroyWindow(xp->d, win[1]);
}

