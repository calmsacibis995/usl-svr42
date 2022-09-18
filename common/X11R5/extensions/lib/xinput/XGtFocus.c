/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGtFocus.c	1.1"
/* $XConsortium: XGtFocus.c,v 1.4 89/12/06 20:38:40 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGetDeviceFocus - Get the focus of an input device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XGetDeviceFocus (dpy, dev, focus, revert_to, time)
    register Display *dpy;
    XDevice *dev;
    Window *focus;
    int *revert_to;
    int *time;
    {       
    xGetDeviceFocusReq 	*req;
    xGetDeviceFocusReply 	rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(GetDeviceFocus,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetDeviceFocus;
    req->deviceid = dev->device_id;

    (void) _XReply (dpy, (xReply *) &rep, 0, xTrue);
    *focus = rep.focus;
    *revert_to = rep.revertTo;
    *time = rep.time;
    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }

