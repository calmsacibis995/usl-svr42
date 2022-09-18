/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XChgKbd.c	1.1"
/* $XConsortium: XChgKbd.c,v 1.6 91/07/23 12:25:50 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XChangeKeyboardDevice - Change the device used as the X keyboard.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XChangeKeyboardDevice (dpy, dev)
    register Display 	*dpy;
    XDevice		*dev;
    {       
    xChangeKeyboardDeviceReq 	*req;
    xChangeKeyboardDeviceReply 	rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(ChangeKeyboardDevice,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_ChangeKeyboardDevice;
    req->deviceid = dev->device_id;
    rep.status = Success;

    (void) _XReply (dpy, (xReply *) &rep, 0, xTrue);

    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.status);
    }

