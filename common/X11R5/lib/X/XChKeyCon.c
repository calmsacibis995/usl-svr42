/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XChKeyCon.c	1.1"
/* $XConsortium: XChKeyCon.c,v 11.12 91/01/26 14:12:12 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

XChangeKeyboardControl(dpy, mask, value_list)
    register Display *dpy;
    unsigned long mask;
    XKeyboardControl *value_list;
{
    unsigned long values[8];
    register unsigned long *value = values;
    long nvalues;
    register xChangeKeyboardControlReq *req;

    LockDisplay(dpy);
    GetReq(ChangeKeyboardControl, req);
    req->mask = mask;

    /* Warning!  This code assumes that "unsigned long" is 32-bits wide */

    if (mask & KBKeyClickPercent)
	*value++ = value_list->key_click_percent;
	
    if (mask & KBBellPercent)
    	*value++ = value_list->bell_percent;

    if (mask & KBBellPitch)
    	*value++ = value_list->bell_pitch;

    if (mask & KBBellDuration)
    	*value++ = value_list->bell_duration;

    if (mask & KBLed)
    	*value++ = value_list->led;

    if (mask & KBLedMode)
	*value++ = value_list->led_mode;

    if (mask & KBKey)
        *value++ = value_list->key;

    if (mask & KBAutoRepeatMode)
        *value++ = value_list->auto_repeat_mode;


    req->length += (nvalues = value - values);

    /* note: Data is a macro that uses its arguments multiple
       times, so "nvalues" is changed in a separate assignment
       statement */

    nvalues <<= 2;
    Data32 (dpy, (long *) values, nvalues);
    UnlockDisplay(dpy);
    SyncHandle();

    }
