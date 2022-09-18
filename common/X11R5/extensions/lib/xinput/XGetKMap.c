/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGetKMap.c	1.1"
/* $XConsortium: XGetKMap.c,v 1.5 90/05/18 11:23:24 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGetDeviceKeyMapping - get the keymap of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

KeySym 
*XGetDeviceKeyMapping (dpy, dev, first, keycount, syms_per_code)
    register	Display 	*dpy;
    XDevice			*dev;
    KeyCode			first;
    int				keycount;
    int				*syms_per_code;		/* RETURN */
    {
    long nbytes;
    register KeySym *mapping = NULL;
    xGetDeviceKeyMappingReq *req;
    xGetDeviceKeyMappingReply rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return ((KeySym *) NoSuchExtension);

    GetReq(GetDeviceKeyMapping,req);
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetDeviceKeyMapping;
    req->deviceid = dev->device_id;
    req->firstKeyCode = first;
    req->count = keycount;

    if (! _XReply (dpy, (xReply *) &rep, 0, xFalse)) 
	{
	UnlockDisplay(dpy);
	SyncHandle();
	return (KeySym *) NULL;
	}
    if (rep.length > 0) {
        *syms_per_code = rep.keySymsPerKeyCode;
	nbytes = (long)rep.length << 2;
	mapping = (KeySym *) Xmalloc((unsigned) nbytes);
	if (mapping)
	    _XRead (dpy, (char *)mapping, nbytes);
	else
	    _XEatData (dpy, (unsigned long) nbytes);
      }

    UnlockDisplay(dpy);
    SyncHandle();
    return (mapping);
    }
