/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xgetvers.c	1.1"
/* $XConsortium: xgetvers.c,v 1.5 90/05/18 15:35:29 rws Exp $ */

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
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();
XExtensionVersion	AllExtensionVersions[128];

/***********************************************************************
 *
 * Handle a request from a client with a different byte order than us.
 *
 */

int
SProcXGetExtensionVersion(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xGetExtensionVersionReq);
    swaps(&stuff->length, n);
    swaps(&stuff->nbytes, n);
    return(ProcXGetExtensionVersion(client));
    }

/***********************************************************************
 *
 * This procedure lists the input devices available to the server.
 *
 */

ProcXGetExtensionVersion (client)
    register ClientPtr client;
    {
    xGetExtensionVersionReply	rep;

    REQUEST(xGetExtensionVersionReq);
    REQUEST_AT_LEAST_SIZE(xGetExtensionVersionReq);


    rep.repType = X_Reply;
    rep.RepType = X_GetExtensionVersion;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.major_version = 0;
    rep.minor_version = 0;

    rep.present = TRUE;
    if (rep.present)
	{
	rep.major_version = 
	    AllExtensionVersions[IReqCode-128].major_version;
	rep.minor_version = 
	    AllExtensionVersions[IReqCode-128].minor_version;
	}
    WriteReplyToClient (client, sizeof (xGetExtensionVersionReply), &rep);

    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XGetExtensionVersion function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXGetExtensionVersion (client, size, rep)
    ClientPtr	client;
    int		size;
    xGetExtensionVersionReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    swaps(&rep->major_version, n);
    swaps(&rep->minor_version, n);
    WriteToClient(client, size, rep);
    }
