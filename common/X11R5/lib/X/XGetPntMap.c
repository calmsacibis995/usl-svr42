/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetPntMap.c	1.1"
/* $XConsortium: XGetPntMap.c,v 1.14 91/01/06 11:46:14 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

#ifdef MIN		/* some systems define this in <sys/param.h> */
#undef MIN
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int XGetPointerMapping (dpy, map, nmaps)
    register Display *dpy;
    unsigned char *map;	/* RETURN */
    int nmaps;

{
    unsigned char mapping[256];	/* known fixed size */
    long nbytes;
    xGetPointerMappingReply rep;
    register xReq *req;

    LockDisplay(dpy);
    GetEmptyReq(GetPointerMapping, req);
    (void) _XReply(dpy, (xReply *)&rep, 0, xFalse);

    nbytes = (long)rep.length << 2;
    _XRead (dpy, (char *)mapping, nbytes);
    /* don't return more data than the user asked for. */
    if (rep.nElts) {
	    bcopy ((char *) mapping, (char *) map, 
		MIN((int)rep.nElts, nmaps) );
	}
    UnlockDisplay(dpy);
    SyncHandle();
    return ((int) rep.nElts);
}

#if NeedFunctionPrototypes
KeySym *XGetKeyboardMapping (Display *dpy,
#if NeedWidePrototypes
			     unsigned int first_keycode,
#else
			     KeyCode first_keycode,
#endif
			     int count,
			     int *keysyms_per_keycode)
#else
KeySym *XGetKeyboardMapping (dpy, first_keycode, count, keysyms_per_keycode)
    register Display *dpy;
    KeyCode first_keycode;
    int count;
    int *keysyms_per_keycode;		/* RETURN */
#endif
{
    long nbytes;
    unsigned long nkeysyms;
    register KeySym *mapping = NULL;
    xGetKeyboardMappingReply rep;
    register xGetKeyboardMappingReq *req;

    LockDisplay(dpy);
    GetReq(GetKeyboardMapping, req);
    req->firstKeyCode = first_keycode;
    req->count = count;
    if (! _XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return (KeySym *) NULL;
    }

    nkeysyms = (unsigned long) rep.length;
    if (nkeysyms > 0) {
	nbytes = nkeysyms * sizeof (KeySym);
	mapping = (KeySym *) Xmalloc ((unsigned) nbytes);
	nbytes = nkeysyms << 2;
	if (! mapping) {
	    _XEatData(dpy, (unsigned long) nbytes);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (KeySym *) NULL;
	}
	_XRead32 (dpy, (char *) mapping, nbytes);
    }
    *keysyms_per_keycode = rep.keySymsPerKeyCode;
    UnlockDisplay(dpy);
    SyncHandle();
    return (mapping);
}

