/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xselectev.c	1.1"
/* $XConsortium: xselectev.c,v 1.9 90/05/18 15:35:37 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Request to select input from an extension device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES

#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "windowstr.h"			/* window structure  */
#include "XI.h"
#include "XIproto.h"

extern	int 		IReqCode;
extern	Mask		ExtExclusiveMasks[];
extern	Mask		ExtValidMasks[];
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * Handle requests from clients with a different byte order.
 *
 */

int
SProcXSelectExtensionEvent (client)
register ClientPtr client;
    {
    register char n;
    register long *p;
    register int i;

    REQUEST(xSelectExtensionEventReq);
    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    swaps(&stuff->count, n);
    p = (long *) &stuff[1];
    for (i=0; i<stuff->count; i++)
        {
        swapl(p, n);
	p++;
        }
    return(ProcXSelectExtensionEvent(client));
    }

/***********************************************************************
 *
 * This procedure selects input from an extension device.
 *
 */

int
ProcXSelectExtensionEvent (client)
    register ClientPtr client;
    {
    int			ret;
    int			i;
    WindowPtr 		pWin;
    struct tmask	tmp[EMASKSIZE];

    REQUEST(xSelectExtensionEventReq);
    REQUEST_AT_LEAST_SIZE(xSelectExtensionEventReq);

    if (stuff->length !=(sizeof(xSelectExtensionEventReq)>>2) + stuff->count)
	{
	SendErrorToClient (client, IReqCode, X_SelectExtensionEvent, 0, 
		BadLength);
	return Success;
	}

    pWin = (WindowPtr) LookupWindow (stuff->window, client);
    if (!pWin)
        {
	client->errorValue = stuff->window;
	SendErrorToClient(client, IReqCode, X_SelectExtensionEvent, 0, 
		BadWindow);
	return Success;
        }

    if ((ret = CreateMaskFromList (client, (XEventClass *)&stuff[1], 
	stuff->count, tmp, NULL, X_SelectExtensionEvent)) != Success)
	return Success;

    for (i=0; i<EMASKSIZE; i++)
	if (tmp[i].dev != NULL)
	    {
	    if ((ret = SelectForWindow(tmp[i].dev, pWin, client, tmp[i].mask, 
		ExtExclusiveMasks[i], ExtValidMasks[i])) != Success)
		{
		SendErrorToClient(client, IReqCode, X_SelectExtensionEvent, 0, 
			ret);
		return Success;
		}
	    }

    return Success;
    }
