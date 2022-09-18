/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetPCnt.c	1.1"
/* $XConsortium: XGetPCnt.c,v 11.11 91/01/06 11:46:12 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

XGetPointerControl(dpy, accel_numer, accel_denom, threshold)
     register Display *dpy;
     /* the following are return only vars */
     int *accel_numer, *accel_denom;
     int *threshold;
{       
    xGetPointerControlReply rep;
    xReq *req;
    LockDisplay(dpy);
    GetEmptyReq(GetPointerControl, req);
    (void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
    *accel_numer = rep.accelNumerator;
    *accel_denom = rep.accelDenominator;
    *threshold = rep.threshold;
    UnlockDisplay(dpy);
    SyncHandle();
}

