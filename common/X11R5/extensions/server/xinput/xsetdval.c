/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xsetdval.c	1.1"
/* $XConsortium: xsetdval.c,v 1.1 91/02/22 15:34:21 rws Exp $ */

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
SProcXSetDeviceValuators(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xSetDeviceValuatorsReq);
    swaps(&stuff->length, n);
    return(ProcXSetDeviceValuators(client));
    }

/***********************************************************************
 *
 * This procedure sets the value of valuators on an extension input device.
 *
 */

int
ProcXSetDeviceValuators(client)
    register ClientPtr client;
    {
    DeviceIntPtr dev;
    xSetDeviceValuatorsReply	rep;

    REQUEST(xSetDeviceValuatorsReq);
    REQUEST_AT_LEAST_SIZE(xSetDeviceValuatorsReq);

    rep.repType = X_Reply;
    rep.RepType = X_SetDeviceValuators;
    rep.length = 0;
    rep.status = Success;
    rep.sequenceNumber = client->sequence;

    if (stuff->length !=(sizeof(xSetDeviceValuatorsReq)>>2) + 
	stuff->num_valuators)
	{
	SendErrorToClient (client, IReqCode, X_SetDeviceValuators, 0, 
		BadLength);
	return Success;
	}
    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient (client, IReqCode, X_SetDeviceValuators, 0, 
	    BadDevice);
	return Success;
	}
    if (dev->valuator == NULL)
	{
	SendErrorToClient(client, IReqCode, X_SetDeviceValuators, 0, 
		BadMatch);
	return Success;
	}

    if (stuff->first_valuator + stuff->num_valuators > dev->valuator->numAxes)
	{
	SendErrorToClient(client, IReqCode, X_SetDeviceValuators, 0, 
		BadValue);
	return Success;
	}

    if ((dev->grab) && !SameClient(dev->grab, client))
	rep.status = AlreadyGrabbed;
    else
	{
	rep.status = SetDeviceValuators (client, dev, (int *) &stuff[1],
	    stuff->first_valuator, stuff->num_valuators);
	if (rep.status != Success)
	    SendErrorToClient(client, IReqCode, X_SetDeviceValuators, 0, 
		rep.status);
	else
	    WriteReplyToClient (client, sizeof (xSetDeviceValuatorsReply), &rep);
	}

    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XSetDeviceValuators function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXSetDeviceValuators (client, size, rep)
    ClientPtr	client;
    int		size;
    xSetDeviceValuatorsReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, rep);
    }
