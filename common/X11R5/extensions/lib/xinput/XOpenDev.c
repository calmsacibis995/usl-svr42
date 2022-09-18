/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XOpenDev.c	1.1"
/* $XConsortium: XOpenDev.c,v 1.6 91/07/23 12:28:39 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XOpenDevice - Request the server to open and extension input device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "XInput.h"
#include "Xlibint.h"
#include "extutil.h"

XDevice
*XOpenDevice(dpy, id)
    register Display 	*dpy;
    register XID	id;
    {	
    register long	rlen;
    xOpenDeviceReq 	*req;
    xOpenDeviceReply 	rep;
    XDevice 		*dev;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return ((XDevice *) NoSuchExtension);

    GetReq(OpenDevice,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_OpenDevice;
    req->deviceid = id;

    if (! _XReply (dpy, (xReply *) &rep, 0, xFalse)) 
	{
	UnlockDisplay(dpy);
	SyncHandle();
	return (XDevice *) NULL;
	}

    rlen = rep.length << 2;
    dev = (XDevice *) Xmalloc (sizeof(XDevice) + rep.num_classes * 
	sizeof (XInputClassInfo));
    if (dev)
	{
	dev->device_id = req->deviceid;
	dev->num_classes = rep.num_classes;
	dev->classes = (XInputClassInfo *) ((char *) dev + sizeof (XDevice));
	_XRead (dpy, dev->classes, rlen);
	}
    else
	_XEatData (dpy, (unsigned long) rlen);

    UnlockDisplay (dpy);
    SyncHandle();
    return (dev);
    }

