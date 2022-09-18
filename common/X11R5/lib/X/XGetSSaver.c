/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetSSaver.c	1.1"
/* $XConsortium: XGetSSaver.c,v 11.11 91/01/06 11:46:18 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#define NEED_REPLIES
#include "Xlibint.h"

XGetScreenSaver(dpy, timeout, interval, prefer_blanking, allow_exp)
     register Display *dpy;
     /* the following are return only vars */
     int *timeout, *interval;
     int *prefer_blanking, *allow_exp;  /*boolean */
     
{       
    xGetScreenSaverReply rep;
    register xReq *req;
    LockDisplay(dpy);
    GetEmptyReq(GetScreenSaver, req);

    (void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
    *timeout = rep.timeout;
    *interval = rep.interval;
    *prefer_blanking = rep.preferBlanking;
    *allow_exp = rep.allowExposures;
    UnlockDisplay(dpy);
    SyncHandle();
}

