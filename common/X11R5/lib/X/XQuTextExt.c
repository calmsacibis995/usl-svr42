/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XQuTextExt.c	1.1"
/* $XConsortium: XQuTextExt.c,v 11.18 91/01/06 11:47:37 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986, 1987	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

#if NeedFunctionPrototypes
XQueryTextExtents (
    register Display *dpy,
    Font fid,
    register _Xconst char *string,
    register int nchars,
    int *dir,
    int *font_ascent,
    int *font_descent,
    register XCharStruct *overall)
#else
XQueryTextExtents (dpy, fid, string, nchars, dir, font_ascent, font_descent,
                   overall)
    register Display *dpy;
    Font fid;
    register char *string;
    register int nchars;
    int *dir;
    int *font_ascent, *font_descent;
    register XCharStruct *overall;
#endif
{
    register int i;
    register char *ptr;
    char *buf;
    xQueryTextExtentsReply rep;
    long nbytes;
    register xQueryTextExtentsReq *req;

    LockDisplay(dpy);
    nbytes = nchars << 1;
    if (! (buf = _XAllocScratch (dpy, (unsigned long) nbytes))) {
	UnlockDisplay(dpy);
	SyncHandle();
	return 0;
    }
    GetReq(QueryTextExtents, req);
    req->fid = fid;
    req->length += (nbytes + 3)>>2;
    req->oddLength = nchars & 1;
    for (ptr = buf, i = nchars; --i >= 0;) {
	*ptr++ = 0;
	*ptr++ = *string++;
    }
    Data (dpy, buf, nbytes);
    if (!_XReply (dpy, (xReply *)&rep, 0, xTrue)) {
        UnlockDisplay(dpy);
	SyncHandle();
	return (0);
    }
    *dir = rep.drawDirection;
    *font_ascent = cvtINT16toInt (rep.fontAscent);
    *font_descent = cvtINT16toInt (rep.fontDescent);
    overall->ascent = (short) cvtINT16toShort (rep.overallAscent);
    overall->descent = (short) cvtINT16toShort (rep.overallDescent);
    /* XXX bogus - we're throwing away information!!! */
    overall->width  = (short) cvtINT32toInt (rep.overallWidth);
    overall->lbearing = (short) cvtINT32toInt (rep.overallLeft);
    overall->rbearing = (short) cvtINT32toInt (rep.overallRight);
    UnlockDisplay(dpy);
    SyncHandle();
    return (1);
}
