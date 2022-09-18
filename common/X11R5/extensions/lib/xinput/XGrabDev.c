/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGrabDev.c	1.1"
/* $XConsortium: XGrabDev.c,v 1.5 91/07/23 12:27:52 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGrabDevice - grab an extension input device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int 
XGrabDevice (dpy, dev, grab_window, ownerEvents, event_count, event_list,
	this_device_mode, other_devices_mode, time)
    register Display 	*dpy;
    XDevice		*dev;
    Window 		grab_window;
    Bool 		ownerEvents;
    int			event_count;
    XEventClass		*event_list;
    int 		this_device_mode;
    int 		other_devices_mode;
    Time 		time;
    {
    xGrabDeviceReply rep;
    register xGrabDeviceReq *req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(GrabDevice,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GrabDevice;
    
    req->deviceid = dev->device_id;
    req->grabWindow = grab_window;
    req->ownerEvents = ownerEvents;
    req->event_count = event_count;
    req->this_device_mode = this_device_mode;
    req->other_devices_mode = other_devices_mode;
    req->time = time;
    req->length += event_count;

    /* note: Data is a macro that uses its arguments multiple
       times, so "nvalues" is changed in a separate assignment
       statement */

    event_count <<= 2;
    Data (dpy, (char *) event_list, event_count);

    if (_XReply (dpy, (xReply *) &rep, 0, xTrue) == 0) 
	rep.status = GrabSuccess;
    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.status);
    }
