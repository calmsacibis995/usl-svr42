/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xungrdev.c	1.1"
/* $XConsortium: xungrdev.c,v 1.6 89/12/02 15:21:45 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Request to release a grab of an extension device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "windowstr.h"			/* window structure  */
#include "XIproto.h"

extern	int 	IReqCode;
extern	int	BadDevice;
extern	void	(* ReplySwapVector[256]) ();
DeviceIntPtr	LookupDeviceIntRec();

/***********************************************************************
 *
 * Handle requests from a client with a different byte order.
 *
 */

int
SProcXUngrabDevice(client)
register ClientPtr client;
    {
    register char n;

    REQUEST(xUngrabDeviceReq);
    swaps(&stuff->length, n);
    swapl(&stuff->time, n);
    return(ProcXUngrabDevice(client));
    }

/***********************************************************************
 *
 * Release a grab of an extension device.
 *
 */

int
ProcXUngrabDevice(client)
register ClientPtr client;
    {
    DeviceIntPtr 	dev;
    GrabPtr 		grab;
    TimeStamp 		time;

    REQUEST(xUngrabDeviceReq);
    REQUEST_SIZE_MATCH(xUngrabDeviceReq);

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient(client, IReqCode, X_UngrabDevice, 0, BadDevice);
	return Success;
	}
    grab =  dev->grab;

    time = ClientTimeToServerTime(stuff->time);
    if ((CompareTimeStamps(time, currentTime) != LATER) &&
	(CompareTimeStamps(time, dev->grabTime) != EARLIER) &&
	(grab) && SameClient(grab, client))
	(*dev->DeactivateGrab)(dev);
    return Success;
    }
