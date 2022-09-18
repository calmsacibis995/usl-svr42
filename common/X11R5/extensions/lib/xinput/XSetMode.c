/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XSetMode.c	1.1"
/* $Header: XSetMode.c,v 1.6 91/02/22 15:25:50 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XSetDeviceMode - Set the mode of an extension input device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XSetDeviceMode (dpy, dev, mode)
    register Display 	*dpy;
    XDevice 		*dev;
    int			mode;
    {       
    xSetDeviceModeReq 		*req;
    xSetDeviceModeReply 	rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(SetDeviceMode,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_SetDeviceMode;
    req->deviceid = dev->device_id;
    req->mode = mode;

    (void) _XReply (dpy, (xReply *) &rep, 0, xTrue);
    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.status);
    }

