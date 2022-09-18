/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xgetprop.c	1.1"
/* $Header: xgetprop.c,v 1.7 91/01/24 17:03:42 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * Function to return the dont-propagate-list for an extension device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "windowstr.h"			/* window structs    */
#include "XI.h"
#include "XIproto.h"

extern	int 		IReqCode;
extern	void		(* ReplySwapVector[256]) ();
extern			XExtEventInfo EventInfo[];
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * Handle a request from a client with a different byte order.
 *
 */

int
SProcXGetDeviceDontPropagateList(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xGetDeviceDontPropagateListReq);
    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    return(ProcXGetDeviceDontPropagateList(client));
    }

/***********************************************************************
 *
 * This procedure lists the input devices available to the server.
 *
 */

ProcXGetDeviceDontPropagateList (client)
    register ClientPtr client;
    {
    CARD16				count = 0;
    int					i;
    XEventClass				*buf, *tbuf;
    WindowPtr 				pWin;
    xGetDeviceDontPropagateListReply	rep;
    XEventClass 			*ClassFromMask ();
    void				Swap32Write();
    OtherInputMasks			*others;

    REQUEST(xGetDeviceDontPropagateListReq);
    REQUEST_SIZE_MATCH(xGetDeviceDontPropagateListReq);

    rep.repType = X_Reply;
    rep.RepType = X_GetDeviceDontPropagateList;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.count = 0;

    pWin = (WindowPtr) LookupWindow (stuff->window, client);
    if (!pWin)
        {
	client->errorValue = stuff->window;
	SendErrorToClient(client, IReqCode, X_GetDeviceDontPropagateList, 0, 
		BadWindow);
	return Success;
        }

    if (others = wOtherInputMasks(pWin))
	{
	for (i=0; i<EMASKSIZE; i++)
	    tbuf = ClassFromMask (NULL, others->dontPropagateMask[i], i, 
		&count, COUNT);
	if (count)
	    {
	    rep.count = count;
	    buf = (XEventClass *) Xalloc (rep.count * sizeof(XEventClass));
	    rep.length = (rep.count * sizeof (XEventClass) + 3) >> 2;

	    tbuf = buf;
	    for (i=0; i<EMASKSIZE; i++)
		tbuf = ClassFromMask (tbuf, others->dontPropagateMask[i], i, 
		    NULL, CREATE);
	    }
	}

    WriteReplyToClient (client, sizeof (xGetDeviceDontPropagateListReply), 
	&rep);

    if (count)
	{
	client->pSwapReplyFunc = Swap32Write;
	WriteSwappedDataToClient( client, count * sizeof(XEventClass), buf);
	Xfree (buf);
	}
    return Success;
    }

/***********************************************************************
 *
 * This procedure gets a list of event classes from a mask word.
 * A single mask may translate to more than one event class.
 *
 */

XEventClass
*ClassFromMask (buf, mask, maskndx, count, mode)
    XEventClass *buf;
    Mask	mask;
    int		maskndx;
    CARD16	*count;
    int		mode;
    {
    int		i,j;
    int		id = maskndx;
    Mask	tmask = 0x80000000;
    extern int	ExtEventIndex;

    for (i=0; i<32; i++,tmask>>=1)
	if (tmask & mask)
	    {
	    for (j=0; j<ExtEventIndex; j++)
		if (EventInfo[j].mask == tmask)
		    {
		    if (mode == COUNT)
			(*count)++;
		    else
		        *buf++ = (id << 8) | EventInfo[j].type;
		    }
	    }
    return (buf);
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XGetDeviceDontPropagateList function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXGetDeviceDontPropagateList (client, size, rep)
    ClientPtr	client;
    int		size;
    xGetDeviceDontPropagateListReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    swaps(&rep->count, n);
    WriteToClient(client, size, rep);
    }
