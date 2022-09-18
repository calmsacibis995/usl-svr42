/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XLiICmaps.c	1.1"
/* $XConsortium: XLiICmaps.c,v 11.17 91/01/06 11:46:47 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

Colormap *XListInstalledColormaps(dpy, win, n)
register Display *dpy;
Window win;
int *n;  /* RETURN */
{
    long nbytes;
    Colormap *cmaps;
    xListInstalledColormapsReply rep;
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(ListInstalledColormaps, win, req);

    if(_XReply(dpy, (xReply *) &rep, 0, xFalse) == 0) {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    *n = 0;
	    return((Colormap *) NULL);
	}

    if (rep.nColormaps) {
	nbytes = rep.nColormaps * sizeof(Colormap);
	cmaps = (Colormap *) Xmalloc((unsigned) nbytes);
	nbytes = rep.nColormaps << 2;
	if (! cmaps) {
	    _XEatData(dpy, (unsigned long) nbytes);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return((Colormap *) NULL);
	}
	_XRead32 (dpy, (char *) cmaps, nbytes);
    }
    else cmaps = (Colormap *) NULL;
    
    *n = rep.nColormaps;
    UnlockDisplay(dpy);
    SyncHandle();
    return(cmaps);
}

