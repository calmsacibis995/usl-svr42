/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XCloseDev.c	1.1"
/* $XConsortium: XCloseDev.c,v 1.5 89/12/13 20:05:46 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XCloseDevice - Request the server to close an extension device.
 *
 */

#include "XIproto.h"
#include "Xlibint.h"
#include "XI.h"
#include "XInput.h"
#include "extutil.h"

int
XCloseDevice(dpy, dev)
    register Display 	*dpy;
    register XDevice	*dev;
    {	
    xCloseDeviceReq 	*req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(CloseDevice,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_CloseDevice;
    req->deviceid = dev->device_id;

    XFree ((char *)dev);
    UnlockDisplay (dpy);
    SyncHandle();
    return (Success);
    }

