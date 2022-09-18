/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xgetbmap.c	1.1"
/* $XConsortium: xgetbmap.c,v 1.4 89/12/02 15:20:53 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Extension function to return the version of the extension.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "XI.h"
#include "XIproto.h"

extern	int 		IReqCode;
extern	int 		BadDevice;
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * This procedure gets the button mapping for the specified device.
 *
 */

int
SProcXGetDeviceButtonMapping(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xGetDeviceButtonMappingReq);
    swaps(&stuff->length, n);
    return(ProcXGetDeviceButtonMapping(client));
    }

/***********************************************************************
 *
 * This procedure gets the button mapping for the specified device.
 *
 */

ProcXGetDeviceButtonMapping (client)
    register ClientPtr client;
    {
    DeviceIntPtr	dev;
    xGetDeviceButtonMappingReply	rep;
    ButtonClassPtr	b;

    REQUEST(xGetDeviceButtonMappingReq);
    REQUEST_SIZE_MATCH(xGetDeviceButtonMappingReq);

    rep.repType = X_Reply;
    rep.RepType = X_GetDeviceButtonMapping;
    rep.nElts = 0;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient(client, IReqCode, X_GetDeviceButtonMapping, 0, 
		BadDevice);
	return Success;
	}

    b = dev->button;
    if (b == NULL)
	{
	SendErrorToClient(client, IReqCode, X_GetDeviceButtonMapping, 0, 
		BadMatch);
	return Success;
	}
    rep.nElts = b->numButtons;
    rep.length = (rep.nElts + (4-1))/4;
    WriteReplyToClient (client, sizeof (xGetDeviceButtonMappingReply), &rep);
    (void)WriteToClient(client, rep.nElts,
			(char *)&b->map[1]);
    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XGetDeviceButtonMapping function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXGetDeviceButtonMapping (client, size, rep)
    ClientPtr	client;
    int		size;
    xGetDeviceButtonMappingReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, rep);
    }
