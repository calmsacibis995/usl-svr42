/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XConvSel.c	1.1"
/* $XConsortium: XConvSel.c,v 11.7 91/01/06 11:44:45 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XConvertSelection(dpy, selection, target, property, requestor, time)
register Display *dpy;
Atom selection, target;
Atom property;
Window requestor;
Time time;
{
    register xConvertSelectionReq *req;

    LockDisplay(dpy);
    GetReq(ConvertSelection, req);
    req->selection = selection;
    req->target = target;
    req->property = property;
    req->requestor = requestor;
    req->time = time;
    UnlockDisplay(dpy);
    SyncHandle();
}
