/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xgrabdevb.c	1.1"
/* $Header: xgrabdevb.c,v 1.9 91/05/05 18:29:38 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Extension function to grab a button on an extension device.
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

extern	int 		IReqCode;
extern	int		BadDevice;
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * Handle requests from clients with a different byte order.
 *
 */

int
SProcXGrabDeviceButton(client)
    register ClientPtr client;
    {
    register char n;
    register long *p;
    register int i;

    REQUEST(xGrabDeviceButtonReq);
    swaps(&stuff->length, n);
    swapl(&stuff->grabWindow, n);
    swaps(&stuff->modifiers, n);
    swaps(&stuff->event_count, n);
    p = (long *) &stuff[1];
    for (i=0; i<stuff->event_count; i++)
        {
        swapl(p, n);
	p++;
        }

    return(ProcXGrabDeviceButton(client));
    }

/***********************************************************************
 *
 * Grab a button on an extension device.
 *
 */

int
ProcXGrabDeviceButton(client)
    ClientPtr client;
    {
    int			ret;
    DeviceIntPtr	dev;
    DeviceIntPtr	mdev;
    XEventClass		*class;
    struct tmask	tmp[EMASKSIZE];

    REQUEST(xGrabDeviceButtonReq);
    REQUEST_AT_LEAST_SIZE(xGrabDeviceButtonReq);

    if (stuff->length !=(sizeof(xGrabDeviceButtonReq)>>2) + stuff->event_count)
	{
	SendErrorToClient (client, IReqCode, X_GrabDeviceButton, 0, BadLength);
	return Success;
	}

    dev = LookupDeviceIntRec (stuff->grabbed_device);
    if (dev == NULL)
	{
	SendErrorToClient(client, IReqCode, X_GrabDeviceButton, 0, 
	    BadDevice);
	return Success;
	}
    if (stuff->modifier_device != UseXKeyboard)
	{
	mdev = LookupDeviceIntRec (stuff->modifier_device);
	if (mdev == NULL)
	    {
	    SendErrorToClient(client, IReqCode, X_GrabDeviceButton, 0, 
	        BadDevice);
	    return Success;
	    }
	if (mdev->key == NULL)
	    {
	    SendErrorToClient(client, IReqCode, X_GrabDeviceButton, 0, 
		BadMatch);
	    return Success;
	    }
	}
    else
	mdev = (DeviceIntPtr) LookupKeyboardDevice();

    class = (XEventClass *) (&stuff[1]);	/* first word of values */

    if ((ret = CreateMaskFromList (client, class,
	stuff->event_count, tmp, dev, X_GrabDeviceButton)) != Success)
	    return Success;
    ret = GrabButton(client, dev, stuff->this_device_mode, 
	stuff->other_devices_mode, stuff->modifiers, mdev, stuff->button, 
	stuff->grabWindow, stuff->ownerEvents, NullCursor, NullWindow, 
	tmp[stuff->grabbed_device].mask);

    if (ret != Success)
	SendErrorToClient(client, IReqCode, X_GrabDeviceButton, 0, ret);
    return(Success);
    }
