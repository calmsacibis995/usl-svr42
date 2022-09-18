/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Depths.c	1.1"
/* $XConsortium: Depths.c,v 1.6 91/02/01 16:32:55 gildea Exp $ */
/* Copyright 1989 Massachusetts Institute of Technology */

/*
*/

#include "Xlibint.h"
#include <stdio.h>

/*
 * XListDepths - return info from connection setup
 */
int *XListDepths (dpy, scrnum, countp)
    Display *dpy;
    int scrnum;
    int *countp;
{
    Screen *scr;
    int count;
    int *depths;

    if (scrnum < 0 || scrnum >= dpy->nscreens) return NULL;

    scr = &dpy->screens[scrnum];
    if ((count = scr->ndepths) > 0) {
	register Depth *dp;
	register int i;

	depths = (int *) Xmalloc (count * sizeof(int));
	if (!depths) return NULL;
	for (i = 0, dp = scr->depths; i < count; i++, dp++) 
	  depths[i] = dp->depth;
    } else {
	/* a screen must have a depth */
	return NULL;
    }
    *countp = count;
    return depths;
}
