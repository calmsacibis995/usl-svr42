/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XUngrDvB.c	1.1"
/* $Header: XUngrDvB.c,v 1.5 91/01/24 16:09:17 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XUngrabDeviceButton - Ungrab a button on an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XUngrabDeviceButton(dpy, dev, button, modifiers, modifier_dev, grab_window)
    register 	Display 	*dpy;
    XDevice			*dev;
    unsigned 	int 		button; /* CARD8 */
    unsigned 	int 		modifiers; /* CARD16 */
    XDevice			*modifier_dev;
    Window 			grab_window;
    {
    register xUngrabDeviceButtonReq 	*req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay(dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);
    GetReq(UngrabDeviceButton, req);

    req->reqType = info->codes->major_opcode;
    req->ReqType = X_UngrabDeviceButton;
    req->grabbed_device = dev->device_id;
    req->button = button;
    req->modifiers = modifiers;
    if (modifier_dev)
	req->modifier_device = modifier_dev->device_id;
    else
	req->modifier_device = UseXKeyboard;
    req->grabWindow = grab_window;
    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }
