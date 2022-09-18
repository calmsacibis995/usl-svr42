/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XSetMMap.c	1.1"
/* $XConsortium: XSetMMap.c,v 1.4 89/12/06 20:38:53 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XSetDeviceModifierMapping - set the modifier map of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int 
XSetDeviceModifierMapping (dpy, dev, modmap)
    register		Display 	*dpy;
    XDevice				*dev;
    XModifierKeymap			*modmap;
    {
    int         mapSize = modmap->max_keypermod << 3;	/* 8 modifiers */
    xSetDeviceModifierMappingReq 	*req;
    xSetDeviceModifierMappingReply 	rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReqExtra(SetDeviceModifierMapping, mapSize, req);
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_SetDeviceModifierMapping;
    req->deviceid = dev->device_id;
    req->numKeyPerModifier = modmap->max_keypermod;
    bcopy(modmap->modifiermap, (char *)&req[1], mapSize);

    (void) _XReply(dpy, (xReply *) &rep,
	(sizeof(xSetModifierMappingReply) - sizeof(xReply)) >> 2, xTrue);

    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.success);
    }
