/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGetMMap.c	1.1"
/* $XConsortium: XGetMMap.c,v 1.5 90/05/18 11:23:26 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGetDeviceModifierMapping - get the modifier map of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

XModifierKeymap 
*XGetDeviceModifierMapping (dpy, dev)
    register	Display 	*dpy;
    XDevice			*dev;
    {
    unsigned long nbytes;
    XModifierKeymap *res;
    xGetDeviceModifierMappingReq *req;
    xGetDeviceModifierMappingReply rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return ((XModifierKeymap *) NoSuchExtension);

    GetReq(GetDeviceModifierMapping,req);
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetDeviceModifierMapping;
    req->deviceid = dev->device_id;

    if (! _XReply (dpy, (xReply *) &rep, 0, xFalse)) 
	{
	UnlockDisplay(dpy);
	SyncHandle();
	return (XModifierKeymap *) NULL;
	}
    nbytes = (unsigned long)rep.length << 2;
    res = (XModifierKeymap *) Xmalloc(sizeof (XModifierKeymap));
    if (res)
	{
        res->modifiermap = (KeyCode *) Xmalloc (nbytes);
	if (res->modifiermap)
	    _XReadPad(dpy, (char *) res->modifiermap, nbytes);
	else
	    _XEatData (dpy, (unsigned long) nbytes);
	res->max_keypermod = rep.numKeyPerModifier;
	}

    UnlockDisplay(dpy);
    SyncHandle();
    return (res);
    }
