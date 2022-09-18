/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XLiHosts.c	1.1"
/* $XConsortium: XLiHosts.c,v 11.19 91/01/06 11:46:46 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

/* This can really be considered an os dependent routine */

#define NEED_REPLIES
#include "Xlibint.h"
/*
 * can be freed using XFree.
 */

XHostAddress *XListHosts (dpy, nhosts, enabled)
    register Display *dpy;
    int *nhosts;	/* RETURN */
    Bool *enabled;	/* RETURN */
    {
    register XHostAddress *outbuf = 0, *op;
    xListHostsReply reply;
    long nbytes;
    unsigned char *buf, *bp;
    register unsigned i;
    register xListHostsReq *req;

    LockDisplay(dpy);
    GetReq (ListHosts, req);

    if (!_XReply (dpy, (xReply *) &reply, 0, xFalse)) {
       UnlockDisplay(dpy);
       SyncHandle();
       return (XHostAddress *) NULL;
    }

    if (reply.nHosts) {
	nbytes = reply.length << 2;	/* compute number of bytes in reply */
	op = outbuf = (XHostAddress *)
	    Xmalloc((unsigned) (nbytes + reply.nHosts * sizeof(XHostAddress)));

	if (! outbuf) {	
	    _XEatData(dpy, (unsigned long) nbytes);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (XHostAddress *) NULL;
	}
	bp = buf = 
	    ((unsigned char  *) outbuf) + reply.nHosts * sizeof(XHostAddress);

	_XRead (dpy, (char *) buf, nbytes);

	for (i = 0; i < reply.nHosts; i++) {
	    op->family = ((xHostEntry *) bp)->family;
	    op->length =((xHostEntry *) bp)->length; 
	    op->address = (char *) (((xHostEntry *) bp) + 1);
	    bp += SIZEOF(xHostEntry) + (((op->length + 3) >> 2) << 2);
	    op++;
	}
    }

    *enabled = reply.enabled;
    *nhosts = reply.nHosts;
    UnlockDisplay(dpy);
    SyncHandle();
    return (outbuf);
}


    


