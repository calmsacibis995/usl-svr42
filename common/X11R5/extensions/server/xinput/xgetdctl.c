/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xgetdctl.c	1.1"
/* $XConsortium: xgetdctl.c,v 1.1 91/07/24 15:50:52 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/********************************************************************
 *
 *  Get Device control attributes for an extension device.
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
extern	void	(* ReplySwapVector[256]) ();
DeviceIntPtr	LookupDeviceIntRec();
void		CopySwapDeviceResolution();

/***********************************************************************
 *
 * This procedure gets the control attributes for an extension device,
 * for clients on machines with a different byte ordering than the server.
 *
 */

int
SProcXGetDeviceControl(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xGetDeviceControlReq);
    swaps(&stuff->length, n);
    swaps(&stuff->control, n);
    return(ProcXGetDeviceControl(client));
    }

/***********************************************************************
 *
 * Get the state of the specified device control.
 *
 */

ProcXGetDeviceControl(client)
    ClientPtr client;
    {
    int	total_length = 0;
    char *buf, *savbuf;
    register DeviceIntPtr dev;
    xGetDeviceControlReply rep;

    REQUEST(xGetDeviceControlReq);
    REQUEST_SIZE_MATCH(xGetDeviceControlReq);

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient (client, IReqCode, X_GetDeviceControl, 0, 
		BadDevice);
	return Success;
	}

    rep.repType = X_Reply;
    rep.RepType = X_GetDeviceControl;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    switch (stuff->control)
	{
	case DEVICE_RESOLUTION:
	    if (!dev->valuator)
		{
		SendErrorToClient (client, IReqCode, X_GetDeviceControl, 0, 
		    BadMatch);
		return Success;
		}
	    total_length = sizeof (xDeviceResolutionState) +
		(3 * sizeof(int) * dev->valuator->numAxes);
	    break;
	default:
	    SendErrorToClient (client, IReqCode, X_GetDeviceControl, 0, 
		BadValue);
	    return Success;
	}

    buf = (char *) Xalloc (total_length);
    if (!buf)
	{
	SendErrorToClient(client, IReqCode, X_GetDeviceControl, 0, 
		BadAlloc);
	return Success;
	}
    savbuf=buf;

    switch (stuff->control)
	{
	case DEVICE_RESOLUTION:
	    CopySwapDeviceResolution(client, dev->valuator, buf,
		total_length);
	    break;
	default:
	    break;
	}

    rep.length = (total_length+3) >> 2;
    WriteReplyToClient(client, sizeof(xGetDeviceControlReply), &rep);
    WriteToClient(client, total_length, savbuf);
    Xfree (savbuf);
    return Success;
    }

/***********************************************************************
 *
 * This procedure copies DeviceResolution data, swapping if necessary.
 *
 */

void
CopySwapDeviceResolution (client, v, buf, length)
    ClientPtr 		client;
    ValuatorClassPtr	v;
    char 		*buf;
    int			length;
    {
    register char 	n;
    AxisInfoPtr	a;
    xDeviceResolutionState *r;
    int i, *iptr;

    r = (xDeviceResolutionState *) buf;
    r->control = DEVICE_RESOLUTION;
    r->length =  length;
    r->num_valuators =  v->numAxes;
    buf += sizeof (xDeviceResolutionState);
    iptr = (int *) buf;
    for (i=0,a=v->axes; i<v->numAxes; i++,a++)
	*iptr++ = a->resolution;
    for (i=0,a=v->axes; i<v->numAxes; i++,a++)
	*iptr++ = a->min_resolution;
    for (i=0,a=v->axes; i<v->numAxes; i++,a++)
	*iptr++ = a->max_resolution;
    if (client->swapped)
	{
	swaps (&r->control,n);
	swaps (&r->length,n);
	swapl (&r->num_valuators,n);
	iptr = (int *) buf;
	for (i=0; i < (3 * v->numAxes); i++,iptr++)
	    {
	    swapl (iptr,n);
	    }
	}
    }

/***********************************************************************
 *
 * This procedure writes the reply for the xGetDeviceControl function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXGetDeviceControl (client, size, rep)
    ClientPtr	client;
    int		size;
    xGetDeviceControlReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, rep);
    }

