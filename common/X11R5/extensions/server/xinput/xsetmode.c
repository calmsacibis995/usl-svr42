/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xsetmode.c	1.1"
/* $XConsortium: xsetmode.c,v 1.8 91/02/22 15:33:34 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Request to change the mode of an extension input device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "XI.h"
#include "XIproto.h"
#include "inputstr.h"			/* DeviceIntPtr	     */

extern	int 		IReqCode;
extern	int		BadDevice;
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * Handle a request from a client with a different byte order.
 *
 */

int
SProcXSetDeviceMode(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xSetDeviceModeReq);
    swaps(&stuff->length, n);
    return(ProcXSetDeviceMode(client));
    }

/***********************************************************************
 *
 * This procedure sets the mode of a device.
 *
 */

int
ProcXSetDeviceMode(client)
    register ClientPtr client;
    {
    DeviceIntPtr dev;
    xSetDeviceModeReply	rep;

    REQUEST(xSetDeviceModeReq);
    REQUEST_SIZE_MATCH(xSetDeviceModeReq);

    rep.repType = X_Reply;
    rep.RepType = X_SetDeviceMode;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient (client, IReqCode, X_SetDeviceMode, 0, BadDevice);
	return Success;
	}
    if (dev->valuator == NULL)
	{
	SendErrorToClient(client, IReqCode, X_SetDeviceValuators, 0, 
		BadMatch);
	return Success;
	}
    if ((dev->grab) && !SameClient(dev->grab, client))
	rep.status = AlreadyGrabbed;
    else
	{
	rep.status = SetDeviceMode (client, dev, stuff->mode);
	if (rep.status != Success)
	    SendErrorToClient(client, IReqCode, X_SetDeviceMode, 0, rep.status);
	else
	    {
  	    dev->valuator->mode = stuff->mode;
	    WriteReplyToClient (client, sizeof (xSetDeviceModeReply), &rep);
	    }
	}


    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XSetDeviceMode function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXSetDeviceMode (client, size, rep)
    ClientPtr	client;
    int		size;
    xSetDeviceModeReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, rep);
    }
