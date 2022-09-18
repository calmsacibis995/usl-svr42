/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XQuKeybd.c	1.1"
/* $XConsortium: XQuKeybd.c,v 11.11 91/01/06 11:47:33 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

struct kmap {
  char map[32];
};

XQueryKeymap(dpy, keys)
    register Display *dpy;
    char keys[32];

{       
    xQueryKeymapReply rep;
    register xReq *req;

    LockDisplay(dpy);
    GetEmptyReq(QueryKeymap, req);
    (void) _XReply(dpy, (xReply *)&rep, 
       (SIZEOF(xQueryKeymapReply) - SIZEOF(xReply)) >> 2, xTrue);
    *(struct kmap *) keys = *(struct kmap *)rep.map;  /* faster than bcopy */
    UnlockDisplay(dpy);
    SyncHandle();
}

