/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XUngrDev.c	1.1"
/* $XConsortium: XUngrDev.c,v 1.4 89/12/06 20:39:01 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XUngrabDevice - Ungrab an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XUngrabDevice (dpy, dev, time)
    register Display *dpy;
    XDevice *dev;
    Time time;
    {
    register xUngrabDeviceReq 	*req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(UngrabDevice,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_UngrabDevice;
    
    req->deviceid = dev->device_id;
    req->time = time;

    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }
