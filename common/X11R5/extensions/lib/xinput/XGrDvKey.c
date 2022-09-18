/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGrDvKey.c	1.1"
/* $Header: XGrDvKey.c,v 1.5 91/01/24 16:06:51 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGrabDeviceKey - Grab a key on an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XGrabDeviceKey (dpy, dev, key, modifiers, modifier_device, 
	grab_window, owner_events, event_count, event_list, this_device_mode, 
	other_devices_mode)
    register 	Display 	*dpy;
    XDevice			*dev;
    unsigned 	int 		key; /* CARD8 */
    unsigned 	int 		modifiers; /* CARD16 */
    XDevice			*modifier_device;
    Window 			grab_window;
    Bool 			owner_events;
    unsigned 	int 		event_count;
    XEventClass 		*event_list;
    int 			this_device_mode;
    int 			other_devices_mode;
    {
    register xGrabDeviceKeyReq *req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(GrabDeviceKey, req);

    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GrabDeviceKey;
    req->grabbed_device = dev->device_id;
    req->key = key;
    req->modifiers = modifiers;
    if (modifier_device)
	req->modifier_device = modifier_device->device_id;
    else
	req->modifier_device = UseXKeyboard;
    req->grabWindow = grab_window;
    req->ownerEvents = owner_events;
    req->event_count = event_count;
    req->this_device_mode = this_device_mode;
    req->other_devices_mode = other_devices_mode;
    req->length += event_count;

    /* note: Data is a macro that uses its arguments multiple
       times, so "nvalues" is changed in a separate assignment
       statement */

    event_count <<= 2;
    Data (dpy, (char *) event_list, event_count);

    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }
