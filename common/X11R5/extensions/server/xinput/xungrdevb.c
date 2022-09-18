/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xungrdevb.c	1.1"
/* $Header: xungrdevb.c,v 1.7 91/05/05 18:29:43 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Request to release a grab of a button on an extension device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "windowstr.h"			/* window structure  */
#include "XI.h"
#include "XIproto.h"

extern	int 	IReqCode;
extern	int	BadDevice;
extern	int	DeviceButtonPress;
extern	void	(* ReplySwapVector[256]) ();
DeviceIntPtr	LookupDeviceIntRec();

/***********************************************************************
 *
 * Handle requests from a client with a different byte order.
 *
 */

int
SProcXUngrabDeviceButton(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xUngrabDeviceButtonReq);
    swaps(&stuff->length, n);
    swapl(&stuff->grabWindow, n);
    swaps(&stuff->modifiers, n);
    return(ProcXUngrabDeviceButton(client));
    }

/***********************************************************************
 *
 * Release a grab of a button on an extension device.
 *
 */

int
ProcXUngrabDeviceButton(client)
    ClientPtr client;
    {
    DeviceIntPtr	dev;
    DeviceIntPtr	mdev;
    WindowPtr		pWin;
    GrabRec		temporaryGrab;

    REQUEST(xUngrabDeviceButtonReq);
    REQUEST_SIZE_MATCH(xUngrabDeviceButtonReq);

    dev = LookupDeviceIntRec (stuff->grabbed_device);
    if (dev == NULL)
	{
	SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
	    BadDevice);
	return Success;
	}
    if (dev->button == NULL)
	{
	SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
		BadMatch);
	return Success;
	}

    if (stuff->modifier_device != UseXKeyboard)
	{
	mdev = LookupDeviceIntRec (stuff->modifier_device);
	if (mdev == NULL)
	    {
	    SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
	        BadDevice);
	    return Success;
	    }
	if (mdev->key == NULL)
	    {
	    SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
		BadMatch);
	    return Success;
	    }
	}
    else
	mdev = (DeviceIntPtr) LookupKeyboardDevice();

    pWin = LookupWindow(stuff->grabWindow, client);
    if (!pWin)
	{
	SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
	    BadWindow);
	return Success;
	}

    temporaryGrab.resource = client->clientAsMask;
    temporaryGrab.device = dev;
    temporaryGrab.window = pWin;
    temporaryGrab.type = DeviceButtonPress;
    temporaryGrab.modifierDevice = mdev;
    temporaryGrab.modifiersDetail.exact = stuff->modifiers;
    temporaryGrab.modifiersDetail.pMask = NULL;
    temporaryGrab.detail.exact = stuff->button;
    temporaryGrab.detail.pMask = NULL;

    DeletePassiveGrabFromList(&temporaryGrab);
    return Success;
    }
