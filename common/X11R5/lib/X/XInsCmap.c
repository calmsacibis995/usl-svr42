/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XInsCmap.c	1.1"
/* $XConsortium: XInsCmap.c,v 11.7 91/01/06 11:46:39 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XInstallColormap(dpy, cmap)
register Display *dpy;
Colormap cmap;
{
    register xResourceReq *req;
    LockDisplay(dpy);
    GetResReq(InstallColormap, cmap, req);
    UnlockDisplay(dpy);
    SyncHandle();
}

