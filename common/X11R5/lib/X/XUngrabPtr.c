/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XUngrabPtr.c	1.1"
/* $XConsortium: XUngrabPtr.c,v 11.9 91/01/06 11:48:37 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XUngrabPointer(dpy, time)
register Display *dpy;
Time time;
{
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(UngrabPointer, time, req);
    UnlockDisplay(dpy);
    SyncHandle();
}
