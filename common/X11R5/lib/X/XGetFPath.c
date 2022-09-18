/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetFPath.c	1.1"
/* $XConsortium: XGetFPath.c,v 11.15 91/01/06 11:45:55 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

char **XGetFontPath(dpy, npaths)
register Display *dpy;
int *npaths;	/* RETURN */
{
	xGetFontPathReply rep;
	register long nbytes;
	char **flist;
	char *ch;
	register unsigned i;
	register int length;
	register xReq *req;

	LockDisplay(dpy);
	GetEmptyReq (GetFontPath, req);
	(void) _XReply (dpy, (xReply *) &rep, 0, xFalse);

	if (rep.nPaths) {
	    flist = (char **)
		Xmalloc((unsigned) rep.nPaths * sizeof (char *));
	    nbytes = (long)rep.length << 2;
	    ch = (char *) Xmalloc ((unsigned) (nbytes + 1));
                /* +1 to leave room for last null-terminator */

	    if ((! flist) || (! ch)) {
		if (flist) Xfree((char *) flist);
		if (ch) Xfree(ch);
		_XEatData(dpy, (unsigned long) nbytes);
		UnlockDisplay(dpy);
		SyncHandle();
		return (char **) NULL;
	    }

	    _XReadPad (dpy, ch, nbytes);
	    /*
	     * unpack into null terminated strings.
	     */
	    length = *ch;
	    for (i = 0; i < rep.nPaths; i++) {
		flist[i] = ch+1;  /* skip over length */
		ch += length + 1; /* find next length ... */
		length = *ch;
		*ch = '\0'; /* and replace with null-termination */
	    }
	}
	else flist = NULL;
	*npaths = rep.nPaths;
	UnlockDisplay(dpy);
	SyncHandle();
	return (flist);
}

XFreeFontPath (list)
char **list;
{
	if (list != NULL) {
		Xfree (list[0]-1);
		Xfree ((char *)list);
	}
}
