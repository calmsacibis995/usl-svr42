/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XAllPlanes.c	1.1"
/* $XConsortium: XAllPlanes.c,v 11.17 91/01/06 11:44:04 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

Status XAllocColorPlanes(dpy, cmap, contig, pixels, ncolors, nreds, ngreens, 
                         nblues, rmask, gmask, bmask)
register Display *dpy;
Colormap cmap;
Bool contig;
unsigned long *pixels; /* LISTofCARD32 */ /* RETURN */
int ncolors;
int nreds, ngreens, nblues;
unsigned long *rmask, *gmask, *bmask; /* CARD32 */ /* RETURN */
{
    xAllocColorPlanesReply rep;
    Status status;
    register xAllocColorPlanesReq *req;

    LockDisplay(dpy);
    GetReq(AllocColorPlanes,req);

    req->cmap = cmap;
    req->colors = ncolors;
    req->red = nreds;
    req->green = ngreens;
    req->blue = nblues;
    req->contiguous = contig;

    status = _XReply(dpy, (xReply *)&rep, 0, xFalse);


    if (status) {
	*rmask = rep.redMask;
	*gmask = rep.greenMask;
	*bmask = rep.blueMask;

	/* sizeof(CARD32) = 4 */
	_XRead32 (dpy, (char *) pixels, (long)(ncolors * 4));
    }

    UnlockDisplay(dpy);
    SyncHandle();
    return(status);
}    
