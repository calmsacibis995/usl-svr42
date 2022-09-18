/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XAllCells.c	1.1"
/* $XConsortium: XAllCells.c,v 11.19 91/01/06 11:44:03 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES

#include "Xlibint.h"

Status XAllocColorCells(dpy, cmap, contig, masks, nplanes, pixels, ncolors)
register Display *dpy;
Colormap cmap;
Bool contig;
unsigned int ncolors; /* CARD16 */
unsigned int nplanes; /* CARD16 */
unsigned long *masks; /* LISTofCARD32 */ /* RETURN */
unsigned long *pixels; /* LISTofCARD32 */ /* RETURN */
{

    Status status;
    xAllocColorCellsReply rep;
    register xAllocColorCellsReq *req;
    LockDisplay(dpy);
    GetReq(AllocColorCells, req);

    req->cmap = cmap;
    req->colors = ncolors;
    req->planes = nplanes;
    req->contiguous = contig;

    status = _XReply(dpy, (xReply *)&rep, 0, xFalse);

    if (status) {
	_XRead32 (dpy, (long *) pixels, 4L * (long) (rep.nPixels));
	_XRead32 (dpy, (long *) masks, 4L * (long) (rep.nMasks));
    }

    UnlockDisplay(dpy);
    SyncHandle();
    return(status);
}
