/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xsetfocus.c	1.1"
/* $XConsortium: xsetfocus.c,v 1.5 90/05/18 14:15:11 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Request to set the focus for an extension device.
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
extern	InputInfo	inputInfo;
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * This procedure sets the focus for a device.
 *
 */

int
SProcXSetDeviceFocus(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xSetDeviceFocusReq);
    swaps(&stuff->length, n);
    swapl(&stuff->focus, n);
    swapl(&stuff->time, n);
    return(ProcXSetDeviceFocus(client));
    }

/***********************************************************************
 *
 * This procedure sets the focus for a device.
 *
 */

int
ProcXSetDeviceFocus(client)
    register ClientPtr client;
    {
    int				ret;
    register DeviceIntPtr	dev;

    REQUEST(xSetDeviceFocusReq);
    REQUEST_SIZE_MATCH(xSetDeviceFocusReq);

    dev = LookupDeviceIntRec (stuff->device);
    if (dev==NULL || !dev->focus)
	{
	SendErrorToClient(client, IReqCode, X_SetDeviceFocus, 0, BadDevice);
	return Success;
	}

    ret = SetInputFocus (client, dev, stuff->focus, stuff->revertTo, 
	stuff->time, TRUE);
    if (ret != Success)
	SendErrorToClient(client, IReqCode, X_SetDeviceFocus, 0, ret);

    return Success;
    }
