/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XRotProp.c	1.1"
/* $XConsortium: XRotProp.c,v 11.14 91/01/06 11:47:49 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XRotateWindowProperties(dpy, w, properties, nprops, npositions)
    register Display *dpy;
    Window w;
    Atom *properties;
    register int nprops;
    int npositions;
    {
    register long nbytes;
    register xRotatePropertiesReq *req;

    LockDisplay(dpy);
    GetReq (RotateProperties, req);
    req->window = w;
    req->nAtoms = nprops;
    req->nPositions = npositions;
    
    req->length += nprops;
    nbytes = nprops << 2;
/* XXX Cray needs packing here.... */
    Data32 (dpy, (long *) properties, nbytes);


    UnlockDisplay(dpy);
    SyncHandle();
    }





