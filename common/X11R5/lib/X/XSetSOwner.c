/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetSOwner.c	1.1"
/* $XConsortium: XSetSOwner.c,v 11.9 91/01/06 11:48:08 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XSetSelectionOwner(dpy, selection, owner, time)
register Display *dpy;
Atom selection;
Window owner;
Time time;
{
    register xSetSelectionOwnerReq *req;

    LockDisplay(dpy);
    GetReq(SetSelectionOwner,req);
    req->selection = selection;
    req->window = owner;
    req->time = time;
    UnlockDisplay(dpy);
    SyncHandle();
}
