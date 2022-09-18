/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XLiProps.c	1.1"
/* $XConsortium: XLiProps.c,v 11.20 91/01/06 11:46:49 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

Atom *XListProperties(dpy, window, n_props)
register Display *dpy;
Window window;
int *n_props;  /* RETURN */
{
    long nbytes;
    xListPropertiesReply rep;
    Atom *properties;
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(ListProperties, window, req);
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	*n_props = 0;
	UnlockDisplay(dpy);
        SyncHandle();
	return ((Atom *) NULL);
    }

    if (rep.nProperties) {
	nbytes = rep.nProperties * sizeof(Atom);
	properties = (Atom *) Xmalloc ((unsigned) nbytes);
	nbytes = rep.nProperties << 2;
	if (! properties) {
	    _XEatData(dpy, (unsigned long) nbytes);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (Atom *) NULL;
	}
	_XRead32 (dpy, (char *) properties, nbytes);
    }
    else properties = (Atom *) NULL;

    *n_props = rep.nProperties;
    UnlockDisplay(dpy);
    SyncHandle();
    return (properties);
}
