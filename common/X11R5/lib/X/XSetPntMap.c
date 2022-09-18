/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetPntMap.c	1.1"
/* $XConsortium: XSetPntMap.c,v 11.13 91/01/06 11:48:07 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES

#include "Xlibint.h"
/* returns either  DeviceMappingSuccess or DeviceMappingBusy  */

#if NeedFunctionPrototypes
int XSetPointerMapping (
    register Display *dpy,
    _Xconst unsigned char *map,
    int nmaps)
#else
int XSetPointerMapping (dpy, map, nmaps)
    register Display *dpy;
    unsigned char *map;
    int nmaps;
#endif
    {
    register xSetPointerMappingReq *req;
    xSetPointerMappingReply rep;

    LockDisplay(dpy);
    GetReq (SetPointerMapping, req);
    req->nElts = nmaps;
    req->length += (nmaps + 3)>>2;
    Data (dpy, (char *)map, (long) nmaps);
    if (_XReply (dpy, (xReply *)&rep, 0, xFalse) == 0) 
	rep.success = MappingSuccess;
    UnlockDisplay(dpy);
    SyncHandle();
    return ((int) rep.success);
    }

XChangeKeyboardMapping (dpy, first_keycode, keysyms_per_keycode, 
		     keysyms, nkeycodes)
    register Display *dpy;
    int first_keycode;
    int keysyms_per_keycode;
    KeySym *keysyms;
    int nkeycodes;
    {
    register long nbytes;
    register xChangeKeyboardMappingReq *req;

    LockDisplay(dpy);
    GetReq (ChangeKeyboardMapping, req);
    req->firstKeyCode = first_keycode;
    req->keyCodes = nkeycodes;
    req->keySymsPerKeyCode = keysyms_per_keycode;
    req->firstKeyCode = first_keycode;
    req->length += nkeycodes * keysyms_per_keycode;
    nbytes = keysyms_per_keycode * nkeycodes * 4;
    Data32 (dpy, (long *)keysyms, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
    return;
    }
    
