/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XAllowDv.c	1.1"
/* $XConsortium: XAllowDv.c,v 1.5 91/07/23 12:25:25 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XAllowDeviceEvents - Thaw a frozen extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XAllowDeviceEvents (dpy, dev, event_mode, time)
    register Display 	*dpy;
    XDevice		*dev;
    int			event_mode;
    Time		time;
    {       
    xAllowDeviceEventsReq 	*req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(AllowDeviceEvents,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_AllowDeviceEvents;
    req->deviceid = dev->device_id;
    req->mode = event_mode;
    req->time = time;

    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }

