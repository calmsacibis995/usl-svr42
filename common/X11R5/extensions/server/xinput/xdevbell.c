/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xdevbell.c	1.1"
#ifdef XINPUT
/* $Header: xdevbell.c,v 1.2 91/05/14 12:22:22 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Extension function to change the keyboard device.
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
 * This procedure is invoked to swap the request bytes if the server and
 * client have a different byte order.
 *
 */

int
SProcXDeviceBell(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xDeviceBellReq);
    swaps(&stuff->length, n);
    return(ProcXDeviceBell(client));
    }

/***********************************************************************
 *
 * This procedure rings a bell on an extension device.
 *
 */

ProcXDeviceBell (client)
    register ClientPtr client;
    {
    DeviceIntPtr dev;
    KbdFeedbackPtr k;
    BellFeedbackPtr b;
    int base;
    int newpercent;
    CARD8 class;
    pointer ctrl;
    void (*proc)();

    REQUEST(xDeviceBellReq);
    REQUEST_SIZE_MATCH(xDeviceBellReq);

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	client->errorValue = stuff->deviceid;
	SendErrorToClient(client, IReqCode, X_DeviceBell, 0, BadDevice);
	return Success;
	}

    if (stuff->percent < -100 || stuff->percent > 100)
	{
	client->errorValue = stuff->percent;
	SendErrorToClient(client, IReqCode, X_DeviceBell, 0, BadValue);
	return Success;
	}
    if (stuff->feedbackclass == KbdFeedbackClass)
	{
	for (k=dev->kbdfeed; k; k=k->next)
	    if (k->ctrl.id == stuff->feedbackid)
		break;
	if (!k)
	    {
	    client->errorValue = stuff->feedbackid;
	    SendErrorToClient(client, IReqCode, X_DeviceBell, 0, BadValue);
	    return Success;
	    }
	base = k->ctrl.bell;
	proc = k->BellProc;
	ctrl = (pointer) &(k->ctrl);
	class = KbdFeedbackClass;
	}
    else if (stuff->feedbackclass == BellFeedbackClass)
	{
	for (b=dev->bell; b; b=b->next)
	    if (b->ctrl.id == stuff->feedbackid)
		break;
	if (!b)
	    {
	    client->errorValue = stuff->feedbackid;
	    SendErrorToClient(client, IReqCode, X_DeviceBell, 0, BadValue);
	    return Success;
	    }
	base = b->ctrl.percent;
	proc = b->BellProc;
	ctrl = (pointer) &(b->ctrl);
	class = BellFeedbackClass;
	}
    else
	{
	client->errorValue = stuff->feedbackclass;
	SendErrorToClient(client, IReqCode, X_DeviceBell, 0, BadValue);
	return Success;
	}
    newpercent = (base * stuff->percent) / 100;
    if (stuff->percent < 0)
        newpercent = base + newpercent;
    else
    	newpercent = base - newpercent + stuff->percent;
    (*proc)(newpercent, dev, ctrl, class);

    return Success;
    }

#endif /* XINPUT */
