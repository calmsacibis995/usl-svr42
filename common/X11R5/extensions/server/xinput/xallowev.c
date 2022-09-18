/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xallowev.c	1.1"
/* $XConsortium: xallowev.c,v 1.6 89/12/02 15:20:24 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Function to allow frozen events to be routed from extension input devices.
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
 * This procedure allows frozen events to be routed.
 *
 */

int
SProcXAllowDeviceEvents(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xAllowDeviceEventsReq);
    swapl(&stuff->time, n);
    swaps(&stuff->length, n);
    return(ProcXAllowDeviceEvents(client));
    }

/***********************************************************************
 *
 * This procedure allows frozen events to be routed.
 *
 */

int
ProcXAllowDeviceEvents(client)
    register ClientPtr client;
    {
    TimeStamp		time;
    DeviceIntPtr	thisdev;
    void AllowSome ();

    REQUEST(xAllowDeviceEventsReq);
    REQUEST_SIZE_MATCH(xAllowDeviceEventsReq);

    thisdev = LookupDeviceIntRec (stuff->deviceid);
    if (thisdev == NULL)
	{
	SendErrorToClient(client, IReqCode, X_AllowDeviceEvents, 0, BadDevice);
	return Success;
	}
    time = ClientTimeToServerTime(stuff->time);

    switch (stuff->mode)
        {
	case ReplayThisDevice:
	    AllowSome(client, time, thisdev, NOT_GRABBED);
	    break;
	case SyncThisDevice: 
	    AllowSome(client, time, thisdev, FREEZE_NEXT_EVENT);
	    break;
	case AsyncThisDevice: 
	    AllowSome(client, time, thisdev, THAWED);
	    break;
	case AsyncOtherDevices: 
	    AllowSome(client, time, thisdev, THAW_OTHERS);
	    break;
	case SyncAll:
	    AllowSome(client, time, thisdev, FREEZE_BOTH_NEXT_EVENT);
	    break;
	case AsyncAll:
	    AllowSome(client, time, thisdev, THAWED_BOTH);
	    break;
	default: 
	    SendErrorToClient(client, IReqCode, X_AllowDeviceEvents, 0, 
		BadValue);
	    client->errorValue = stuff->mode;
	    return Success;
        }
    return Success;
    }
