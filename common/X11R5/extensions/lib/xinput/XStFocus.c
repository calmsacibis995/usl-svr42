/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XStFocus.c	1.1"
/* $XConsortium: XStFocus.c,v 1.4 89/12/06 20:38:59 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XSetDeviceFocus - Set the focus of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XSetDeviceFocus(dpy, dev, focus, revert_to, time)
    register 	Display *dpy;
    XDevice 	*dev;
    Window 	focus;
    int 	revert_to;
    Time	time;
    {       
    xSetDeviceFocusReq 	*req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);

    GetReq(SetDeviceFocus,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_SetDeviceFocus;
    req->device = dev->device_id;
    req->focus = focus;
    req->revertTo = revert_to;
    req->time = time;

    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }
