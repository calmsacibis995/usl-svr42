/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XSndExEv.c	1.1"
/* $XConsortium: XSndExEv.c,v 1.5 89/12/13 20:28:10 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XSendExtensionEvent - send an extension event to a client.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

extern Status XInputEventToWire();

Status
XSendExtensionEvent (dpy, dev, dest, prop, count, list, event)
    register Display 	*dpy;
    XDevice 		*dev;
    Window 		dest;
    Bool		prop;
    int			count;
    XEventClass		*list;
    XEvent		*event;
    {       
    int				num_events;
    int				ev_size;
    xSendExtensionEventReq 	*req;
    xEvent 			*ev;
    register Status 		(**fp)();
    Status 			status;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    /* call through display to find proper conversion routine */

    fp = &dpy->wire_vec[event->type & 0177];
    if (*fp == NULL) 
	*fp = XInputEventToWire;
    status = (**fp)(dpy, event, &ev, &num_events);

    if (status) 
	{
	GetReq(SendExtensionEvent,req);		
        req->reqType = info->codes->major_opcode;
	req->ReqType = X_SendExtensionEvent;
	req->deviceid = dev->device_id;
	req->destination = dest;
	req->propagate = prop;
	req->count = count;
	req->num_events = num_events;
	ev_size = num_events * sizeof (xEvent);
	req->length += (count + (ev_size >> 2));

	/* note: Data is a macro that uses its arguments multiple
           times, so "count" is changed in a separate assignment
           statement.  Any extra events must be sent before the event
	   list, in order to ensure quad alignment. */

	Data (dpy, (char *) ev, ev_size);

	count <<= 2;
	Data (dpy, (char *) list, count);
	XFree ((char *)ev);
	}

    UnlockDisplay(dpy);
    SyncHandle();
    return (status);
    }

