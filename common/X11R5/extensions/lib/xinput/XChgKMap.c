/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XChgKMap.c	1.1"
/* $XConsortium: XChgKMap.c,v 1.4 89/12/06 20:31:27 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XChangeDeviceKeyMapping - change the keymap of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XChangeDeviceKeyMapping (dpy, dev, first, syms_per_code, keysyms, count)
    register	Display 	*dpy;
    XDevice			*dev;
    int				first;
    int				syms_per_code;
    KeySym			*keysyms;
    int				count;
    {
    register long nbytes;
    xChangeDeviceKeyMappingReq *req;
    XExtDisplayInfo 	*info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(ChangeDeviceKeyMapping,req);
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_ChangeDeviceKeyMapping;
    req->deviceid = dev->device_id;
    req->firstKeyCode = first;
    req->keyCodes = count;
    req->keySymsPerKeyCode = syms_per_code;
    req->length += count * syms_per_code;
    nbytes = syms_per_code * count * sizeof (CARD32);
    Data (dpy, (char *)keysyms, nbytes);

    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }
