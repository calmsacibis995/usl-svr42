/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:ClickTime.c	1.1"
/* $XConsortium: ClickTime.c,v 1.2 91/01/06 13:32:02 rws Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * Contains XtSetMultiClickTime, XtGetMultiClickTime
 */

#include "IntrinsicI.h"

void XtSetMultiClickTime( dpy, time )
    Display *dpy;
    int time;
{
    _XtGetPerDisplay(dpy)->multi_click_time = time;
}


int XtGetMultiClickTime( dpy )
    Display *dpy;
{
    return _XtGetPerDisplay(dpy)->multi_click_time;
}
