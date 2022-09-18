/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XChPntCon.c	1.1"
/* $XConsortium: XChPntCon.c,v 11.9 91/01/06 11:44:21 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XChangePointerControl(dpy, do_acc, do_thresh, acc_numerator,
		      acc_denominator, threshold)
     register Display *dpy;
     Bool do_acc, do_thresh;
     int acc_numerator, acc_denominator, threshold;

{
    register xChangePointerControlReq *req;

    LockDisplay(dpy);
    GetReq(ChangePointerControl, req);
    req->doAccel = do_acc;
    req->doThresh = do_thresh;
    req->accelNum = acc_numerator;
    req->accelDenum = acc_denominator;
    req->threshold = threshold;
    UnlockDisplay(dpy);
    SyncHandle();
}

