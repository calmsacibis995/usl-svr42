/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xchgprop.c	1.1"
/* $Header: xchgprop.c,v 1.9 91/01/24 16:22:19 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Function to modify the dont-propagate-list for an extension input device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "windowstr.h"
#include "XI.h"
#include "XIproto.h"

extern	int 	BadMode;
extern	int 	BadClass;
extern	int 	IReqCode;
DeviceIntPtr	LookupDeviceIntRec();

/***********************************************************************
 *
 * This procedure returns the extension version.
 *
 */

int
SProcXChangeDeviceDontPropagateList(client)
    register ClientPtr client;
    {
    register char n;
    register long *p;
    register int i;

    REQUEST(xChangeDeviceDontPropagateListReq);
    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    swaps(&stuff->count, n);
    p = (long *) &stuff[1];
    for (i=0; i<stuff->count; i++)
        {
        swapl(p, n);
	p++;
        }
    return(ProcXChangeDeviceDontPropagateList(client));
    }

/***********************************************************************
 *
 * This procedure changes the dont-propagate list for the specified window.
 *
 */

ProcXChangeDeviceDontPropagateList (client)
    register ClientPtr client;
    {
    int			i;
    WindowPtr		pWin;
    struct 		tmask tmp[EMASKSIZE];
    OtherInputMasks	*others;

    REQUEST(xChangeDeviceDontPropagateListReq);
    REQUEST_AT_LEAST_SIZE(xChangeDeviceDontPropagateListReq);

    if (stuff->length !=(sizeof(xChangeDeviceDontPropagateListReq)>>2) + 
	stuff->count)
	{
	SendErrorToClient (client, IReqCode, X_ChangeDeviceDontPropagateList, 0,
	    BadLength);
	return Success;
	}

    pWin = (WindowPtr) LookupWindow (stuff->window, client);
    if (!pWin)
        {
	client->errorValue = stuff->window;
	SendErrorToClient(client, IReqCode, X_ChangeDeviceDontPropagateList, 0, 
		BadWindow);
	return Success;
        }

    if (stuff->mode != AddToList && stuff->mode != DeleteFromList)
        {
	client->errorValue = stuff->window;
	SendErrorToClient(client, IReqCode, X_ChangeDeviceDontPropagateList, 0, 
		BadMode);
	return Success;
        }

    if (CreateMaskFromList (client, (XEventClass *)&stuff[1], 
	stuff->count, tmp, NULL, X_ChangeDeviceDontPropagateList) != Success)
	    return Success;

    others = wOtherInputMasks(pWin);
    if (!others && stuff->mode == DeleteFromList)
	return Success;
    for (i=0; i<EMASKSIZE; i++)
	{
	if (tmp[i].mask == 0)
	    continue;

	if (stuff->mode == DeleteFromList)
	    tmp[i].mask = (others->dontPropagateMask[i] & ~tmp[i].mask);
	else if (others)
	    tmp[i].mask |= others->dontPropagateMask[i];

	if (DeviceEventSuppressForWindow (pWin,client,tmp[i].mask,i) != Success)
	    {
	    SendErrorToClient ( client, IReqCode, X_ChangeDeviceDontPropagateList, 0, 
		BadClass);
	    return Success;
	    }
	}

    return Success;
    }
