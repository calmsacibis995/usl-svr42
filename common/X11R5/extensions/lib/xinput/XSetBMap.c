/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XSetBMap.c	1.1"
/* $XConsortium: XSetBMap.c,v 1.4 89/12/06 20:38:51 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XSetDeviceButtonMapping - Set the button mapping of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"
#define NEED_REPLIES

/* returns either  DeviceMappingSuccess or DeviceMappingBusy  */

int 
XSetDeviceButtonMapping (dpy, device, map, nmap)
    register Display 	*dpy;
    XDevice		*device;
    unsigned char 	map[];
    int 		nmap;
    {
    register xSetDeviceButtonMappingReq *req;
    xSetDeviceButtonMappingReply rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay(dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);
    GetReq (SetDeviceButtonMapping, req);
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_SetDeviceButtonMapping;
    req->map_length = nmap;
    req->length += (nmap + 3)>>2;
    req->deviceid = device->device_id;

    Data (dpy, (char *)map, (long) nmap);	/* note that map is char[] */
    if (_XReply (dpy, (xReply *)&rep, 0, xFalse) == 0) /* suppress error   */
	rep.status = MappingSuccess;
    UnlockDisplay(dpy);
    SyncHandle();
    return ((int) rep.status);
    }
