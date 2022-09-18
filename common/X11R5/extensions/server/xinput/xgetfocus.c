/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xgetfocus.c	1.1"
/* $XConsortium: xgetfocus.c,v 1.5 90/05/18 14:14:43 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Extension function to get the focus for an extension device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "windowstr.h"			/* focus struct      */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "XI.h"
#include "XIproto.h"

extern	int 		IReqCode;
extern	int		BadDevice;
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * This procedure gets the focus for a device.
 *
 */

int
SProcXGetDeviceFocus(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xGetDeviceFocusReq);
    swaps(&stuff->length, n);
    return(ProcXGetDeviceFocus(client));
    }

/***********************************************************************
 *
 * This procedure gets the focus for a device.
 *
 */

int
ProcXGetDeviceFocus(client)
    ClientPtr client;
    {
    DeviceIntPtr	dev;
    FocusClassPtr 	focus;
    xGetDeviceFocusReply rep;

    REQUEST(xGetDeviceFocusReq);
    REQUEST_SIZE_MATCH(xGetDeviceFocusReq);

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL || !dev->focus)
	{
	SendErrorToClient(client, IReqCode, X_GetDeviceFocus, 0, BadDevice);
	return Success;
	}

    rep.repType = X_Reply;
    rep.RepType = X_GetDeviceFocus;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    focus = dev->focus;

    if (focus->win == NoneWin)
	rep.focus = None;
    else if (focus->win == PointerRootWin)
	rep.focus = PointerRoot;
    else if (focus->win == FollowKeyboardWin)
	rep.focus = FollowKeyboard;
    else 
	rep.focus = focus->win->drawable.id;

    rep.time = focus->time.milliseconds;
    rep.revertTo = focus->revert;
    WriteReplyToClient (client, sizeof(xGetDeviceFocusReply), &rep);
    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the GetDeviceFocus function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXGetDeviceFocus (client, size, rep)
    ClientPtr	client;
    int		size;
    xGetDeviceFocusReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    swapl(&rep->focus, n);
    swapl(&rep->time, n);
    WriteToClient(client, size, rep);
    }
