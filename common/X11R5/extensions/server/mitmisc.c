/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/mitmisc.c	1.1"
/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology


********************************************************/

/* RANDOM CRUFT! THIS HAS NO OFFICIAL X CONSORTIUM BLESSING */

/* $XConsortium: mitmisc.c,v 1.4 91/06/17 11:36:15 rws Exp $ */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#define _MITMISC_SERVER_
#include "mitmiscstr.h"

extern Bool permitOldBugs;

static unsigned char MITReqCode;
static int ProcMITDispatch(), SProcMITDispatch();
static void MITResetProc();

void
MITMiscExtensionInit()
{
    ExtensionEntry *extEntry, *AddExtension();

    if (extEntry = AddExtension(MITMISCNAME, 0, 0,
				 ProcMITDispatch, SProcMITDispatch,
				 MITResetProc, StandardMinorOpcode))
	MITReqCode = (unsigned char)extEntry->base;
}

/*ARGSUSED*/
static void
MITResetProc (extEntry)
ExtensionEntry	*extEntry;
{
}

static int
ProcMITSetBugMode(client)
    register ClientPtr client;
{
    REQUEST(xMITSetBugModeReq);

    REQUEST_SIZE_MATCH(xMITSetBugModeReq);
    if ((stuff->onOff != xTrue) && (stuff->onOff != xFalse))
    {
	client->errorValue = stuff->onOff;
	return BadValue;
    }
    permitOldBugs = stuff->onOff;
    return(client->noClientException);
}

static int
ProcMITGetBugMode(client)
    register ClientPtr client;
{
    REQUEST(xMITGetBugModeReq);
    xMITGetBugModeReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xMITGetBugModeReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.onOff = permitOldBugs;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    }
    WriteToClient(client, sizeof(xMITGetBugModeReply), (char *)&rep);
    return(client->noClientException);
}

static int
ProcMITDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_MITSetBugMode:
	return ProcMITSetBugMode(client);
    case X_MITGetBugMode:
	return ProcMITGetBugMode(client);
    default:
	return BadRequest;
    }
}

static int
SProcMITSetBugMode(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xMITSetBugModeReq);

    swaps(&stuff->length, n);
    return ProcMITSetBugMode(client);
}

static int
SProcMITGetBugMode(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xMITGetBugModeReq);

    swaps(&stuff->length, n);
    return ProcMITGetBugMode(client);
}

static int
SProcMITDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_MITSetBugMode:
	return SProcMITSetBugMode(client);
    case X_MITGetBugMode:
	return SProcMITGetBugMode(client);
    default:
	return BadRequest;
    }
}
