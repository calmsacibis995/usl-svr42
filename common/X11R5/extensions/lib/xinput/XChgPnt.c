/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XChgPnt.c	1.1"
/* $XConsortium: XChgPnt.c,v 1.6 91/07/23 12:26:14 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XChangePointerDevice - Change the device used as the X Pointer.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XChangePointerDevice (dpy, dev, xaxis, yaxis)
    register Display 	*dpy;
    XDevice		*dev;
    int			xaxis;
    int			yaxis;
    {       
    xChangePointerDeviceReq 	*req;
    xChangePointerDeviceReply 	rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(ChangePointerDevice,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_ChangePointerDevice;
    req->deviceid = dev->device_id;
    req->xaxis = xaxis;
    req->yaxis = yaxis;
    rep.status = Success;

    (void) _XReply (dpy, (xReply *) &rep, 0, xTrue);

    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.status);
    }

