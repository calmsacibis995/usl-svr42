/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xsetmmap.c	1.1"
/* $XConsortium: xsetmmap.c,v 1.6 89/12/02 15:21:38 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/********************************************************************
 *
 *  Set modifier mapping for an extension device.
 *
 */

#define	 NEED_EVENTS			/* for inputstr.h    */
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "XI.h"
#include "XIproto.h"

extern	int 	IReqCode;
extern	int	BadDevice;
extern	int	DeviceMappingNotify;
extern	void	(* ReplySwapVector[256]) ();
DeviceIntPtr	LookupDeviceIntRec();

/***********************************************************************
 *
 * This procedure sets the modifier mapping for an extension device,
 * for clients on machines with a different byte ordering than the server.
 *
 */

int
SProcXSetDeviceModifierMapping(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xSetDeviceModifierMappingReq);
    swaps(&stuff->length, n);
    return(ProcXSetDeviceModifierMapping(client));
    }

/***********************************************************************
 *
 * Set the device Modifier mapping.
 *
 */

ProcXSetDeviceModifierMapping(client)
    ClientPtr client;
    {
    int					ret;
    xSetDeviceModifierMappingReply	rep;
    DeviceIntPtr			dev;
    KeyClassPtr 			kp;
    
    REQUEST(xSetDeviceModifierMappingReq);
    REQUEST_AT_LEAST_SIZE(xSetDeviceModifierMappingReq);

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient (client, IReqCode, X_SetDeviceModifierMapping, 0, 
		BadDevice);
	return Success;
	}

    rep.repType = X_Reply;
    rep.RepType = X_SetDeviceModifierMapping;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    ret = SetModifierMapping(client, dev, stuff->length,
	(sizeof (xSetDeviceModifierMappingReq)>>2), stuff->numKeyPerModifier, 
	&stuff[1], &kp);

    if (ret == MappingSuccess || ret == MappingBusy || ret == MappingFailed)
	{
	rep.success = ret;
        WriteReplyToClient(client, sizeof(xSetDeviceModifierMappingReply),&rep);
	}
    else
	{
	SendErrorToClient (client, IReqCode, X_SetDeviceModifierMapping, 0,ret);
	return Success;
	}

    if (ret == MappingSuccess)
        {
        SendDeviceMappingNotify(MappingModifier, 0, 0, dev);
        }

    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XSetDeviceModifierMapping function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXSetDeviceModifierMapping (client, size, rep)
    ClientPtr	client;
    int		size;
    xSetDeviceModifierMappingReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, rep);
    }

