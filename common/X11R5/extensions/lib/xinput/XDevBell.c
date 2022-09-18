/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XDevBell.c	1.1"
/* $Header: XDevBell.c,v 1.2 91/07/23 12:26:53 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XDeviceBell - Ring a bell on an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XDeviceBell (dpy, dev, feedbackclass, feedbackid, percent)
    register Display 	*dpy;
    XDevice		*dev;
    XID			feedbackclass, feedbackid;
    int			percent;
    {       
    xDeviceBellReq 	*req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Add_XDeviceBell) == -1)
	return (NoSuchExtension);

    GetReq(DeviceBell,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_DeviceBell;
    req->deviceid = dev->device_id;
    req->feedbackclass = feedbackclass;
    req->feedbackid = feedbackid;
    req->percent = percent;

    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }

