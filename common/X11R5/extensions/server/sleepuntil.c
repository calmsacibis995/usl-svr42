/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/sleepuntil.c	1.1"

/* dixsleep.c - implement millisecond timeouts for X clients */

#include "X.h"
#include "Xmd.h"
#include "misc.h"
#include "windowstr.h"
#include "dixstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

typedef struct _Sertafied {
    struct _Sertafied	*next;
    TimeStamp		revive;
    ClientPtr		pClient;
    XID			id;
    void		(*notifyFunc)();
    pointer		closure;
} SertafiedRec, *SertafiedPtr;

static SertafiedPtr pPending;
static RESTYPE	    SertafiedResType;
static Bool	    BlockHandlerRegistered;
static int	    SertafiedGeneration;
static void	    WachetAuf();
static void	    SertafiedDelete();
static void	    SertafiedBlockHandler();
static void	    SertafiedWakeupHandler();

ClientSleepUntil (client, revive, notifyFunc, closure)
    ClientPtr	client;
    TimeStamp	*revive;
    void	(*notifyFunc)();
    pointer	closure;
{
    SertafiedPtr	pRequest, pReq, pPrev;

    if (SertafiedGeneration != serverGeneration)
    {
	SertafiedResType = CreateNewResourceType (SertafiedDelete);
	if (!SertafiedResType)
	    return FALSE;
	SertafiedGeneration = serverGeneration;
	BlockHandlerRegistered = FALSE;
    }
    pRequest = (SertafiedPtr) xalloc (sizeof (SertafiedRec));
    if (!pRequest)
	return FALSE;
    pRequest->pClient = client;
    pRequest->revive = *revive;
    pRequest->id = FakeClientID (client->index);
    pRequest->closure = closure;
    if (!BlockHandlerRegistered)
    {
	if (!RegisterBlockAndWakeupHandlers (SertafiedBlockHandler,
					     SertafiedWakeupHandler,
					     (pointer) 0))
	{
	    xfree (pRequest);
	    return FALSE;
	}
	BlockHandlerRegistered = TRUE;
    }
    pRequest->notifyFunc = 0;
    if (!AddResource (pRequest->id, SertafiedResType, (pointer) pRequest))
	return FALSE;
    if (!notifyFunc)
	notifyFunc = WachetAuf;
    pRequest->notifyFunc = notifyFunc;
    /* Insert into time-ordered queue, with earliest activation time coming first. */
    pPrev = 0;
    for (pReq = pPending; pReq; pReq = pReq->next)
    {
	if (CompareTimeStamps (pReq->revive, *revive) == LATER)
	    break;
	pPrev = pReq;
    }
    if (pPrev)
	pPrev->next = pRequest;
    else
	pPending = pRequest;
    pRequest->next = pReq;
    IgnoreClient (client);
    return TRUE;
}

static void
WachetAuf (client, closure)
    ClientPtr	client;
    pointer	closure;
{
    if (!client->clientGone)
	AttendClient (client);
}


static void
SertafiedDelete (pRequest)
    SertafiedPtr	pRequest;
{
    SertafiedPtr	pReq, pPrev;

    pPrev = 0;
    for (pReq = pPending; pReq; pReq = pReq->next)
	if (pReq == pRequest)
	{
	    if (pPrev)
		pPrev->next = pReq->next;
	    else
		pPending = pReq->next;
	    break;
	}
    if (pRequest->notifyFunc)
	(*pRequest->notifyFunc) (pRequest->pClient, pRequest->closure);
    xfree (pRequest);
}

static void
SertafiedBlockHandler (data, wt, LastSelectMask)
    pointer	    data;		/* unused */
    pointer	    wt;			/* wait time */
    long	    *LastSelectMask;
{
    SertafiedPtr	    pReq, pNext;
    unsigned long	    newdelay, olddelay;
    TimeStamp		    now;

    if (!pPending)
	return;
    now.milliseconds = GetTimeInMillis ();
    now.months = currentTime.months;
    if ((int) (now.milliseconds - currentTime.milliseconds) < 0)
	now.months++;
    for (pReq = pPending; pReq; pReq = pNext)
    {
	pNext = pReq->next;
	if (CompareTimeStamps (pReq->revive, now) == LATER)
	    break;
	FreeResource (pReq->id, RT_NONE);
    }
    pReq = pPending;
    if (!pReq)
	return;
    newdelay = pReq->revive.milliseconds - now.milliseconds;
    AdjustWaitForDelay (wt, newdelay);
}

static void
SertafiedWakeupHandler (data, i, LastSelectMask)
    pointer	    data;
    int		    i;
    long	    *LastSelectMask;
{
    SertafiedPtr	pReq, pNext;
    TimeStamp		now;

    now.milliseconds = GetTimeInMillis ();
    now.months = currentTime.months;
    if ((int) (now.milliseconds - currentTime.milliseconds) < 0)
	now.months++;
    for (pReq = pPending; pReq; pReq = pNext)
    {
	pNext = pReq->next;
	if (CompareTimeStamps (pReq->revive, now) == LATER)
	    break;
	FreeResource (pReq->id, RT_NONE);
    }
    if (!pPending)
    {
	RemoveBlockAndWakeupHandlers (SertafiedBlockHandler,
				      SertafiedWakeupHandler,
				      (pointer) 0);
	BlockHandlerRegistered = FALSE;
    }
}
