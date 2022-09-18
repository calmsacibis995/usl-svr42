/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/dix.h	1.5"

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: dix.h,v 1.56 89/08/20 12:14:05 rws Exp $ */

#ifndef DIX_H
#define DIX_H

#include "gc.h"
#include "window.h"

#define EARLIER -1
#define SAMETIME 0
#define LATER 1

#define NullClient ((ClientPtr) 0)
#define REQUEST(type) \
	register type *stuff = (type *)client->requestBuffer


#define REQUEST_SIZE_MATCH(req)\
    if ((sizeof(req) >> 2) != stuff->length)\
         return(BadLength)

#define REQUEST_AT_LEAST_SIZE(req) \
    if ((sizeof(req) >> 2) > stuff->length )\
         return(BadLength)

#define REQUEST_FIXED_SIZE(req, n)\
    if (((sizeof(req) >> 2) > stuff->length) || \
        (((sizeof(req) + (n) + 3) >> 2) != stuff->length)) \
         return(BadLength)

#define LEGAL_NEW_RESOURCE(id,client)\
    if (!LegalNewID(id,client)) \
    {\
	client->errorValue = id;\
        return(BadIDChoice);\
    }

#ifdef OLDR4
#define LOOKUP_DRAWABLE(did, client)\
    ((client->lastDrawableID == did) ? \
     client->lastDrawable : (DrawablePtr)LookupDrawable(did, client))

#define VERIFY_GC(pGC, rid, client)\
    if (client->lastGCID == rid)\
        pGC = client->lastGC;\
    else\
	pGC = (GC *)LookupIDByType(rid, RT_GC);\
    if (!pGC)\
    {\
	client->errorValue = rid;\
	return (BadGC);\
    }

#define VALIDATE_DRAWABLE_AND_GC(drawID, pDraw, pGC, client)\
    if ((client->lastDrawableID != drawID) || (client->lastGCID != stuff->gc))\
    {\
        if (client->lastDrawableID != drawID)\
    	    pDraw = (DrawablePtr)LookupIDByClass(drawID, RC_DRAWABLE);\
        else\
	    pDraw = client->lastDrawable;\
        if (client->lastGCID != stuff->gc)\
	    pGC = (GC *)LookupIDByType(stuff->gc, RT_GC);\
        else\
            pGC = client->lastGC;\
	if (pDraw && pGC)\
	{\
	    if ((pDraw->type == UNDRAWABLE_WINDOW) ||\
		(pGC->depth != pDraw->depth) ||\
		(pGC->pScreen != pDraw->pScreen))\
		return (BadMatch);\
	    client->lastDrawable = pDraw;\
	    client->lastDrawableID = drawID;\
            client->lastGC = pGC;\
            client->lastGCID = stuff->gc;\
	}\
    }\
    else\
    {\
        pGC = client->lastGC;\
        pDraw = client->lastDrawable;\
    }\
    if (!pDraw)\
    {\
        client->errorValue = drawID; \
	return (BadDrawable);\
    }\
    if (!pGC)\
    {\
        client->errorValue = stuff->gc;\
        return (BadGC);\
    }\
    if (pGC->serialNumber != pDraw->serialNumber)\
	ValidateGC(pDraw, pGC);
#else

/*
 * These macros are yanked from R5 dix.h; MIT fixed a few return value
 * problems; the new return codes match the protocol
 * 3/16/92
 */
/* XXX if you are using this macro, you are probably not generating Match
 * errors where appropriate 
 */
#define LOOKUP_DRAWABLE(did, client)\
    ((client->lastDrawableID == did) ? \
     client->lastDrawable : (DrawablePtr)LookupDrawable(did, client))

#define VERIFY_DRAWABLE(pDraw, did, client)\
    if (client->lastDrawableID == did)\
	pDraw = client->lastDrawable;\
    else \
    {\
	pDraw = (DrawablePtr) LookupIDByClass(did, RC_DRAWABLE);\
	if (!pDraw) \
	{\
	    client->errorValue = did; \
	    return BadDrawable;\
	}\
	if (pDraw->type == UNDRAWABLE_WINDOW)\
	    return BadMatch;\
    }

#define VERIFY_GEOMETRABLE(pDraw, did, client)\
    if (client->lastDrawableID == did)\
	pDraw = client->lastDrawable;\
    else \
    {\
	pDraw = (DrawablePtr) LookupIDByClass(did, RC_DRAWABLE);\
	if (!pDraw) \
	{\
	    client->errorValue = did; \
	    return BadDrawable;\
	}\
    }

#define VERIFY_GC(pGC, rid, client)\
    if (client->lastGCID == rid)\
        pGC = client->lastGC;\
    else\
	pGC = (GC *)LookupIDByType(rid, RT_GC);\
    if (!pGC)\
    {\
	client->errorValue = rid;\
	return (BadGC);\
    }

#define VALIDATE_DRAWABLE_AND_GC(drawID, pDraw, pGC, client)\
    if ((stuff->gc == INVALID) || (client->lastGCID != stuff->gc) ||\
	(client->lastDrawableID != drawID))\
    {\
	VERIFY_GEOMETRABLE(pDraw, drawID, client);\
	VERIFY_GC(pGC, stuff->gc, client);\
	if ((pGC->depth != pDraw->depth) ||\
	    (pGC->pScreen != pDraw->pScreen))\
	    return (BadMatch);\
	client->lastDrawable = pDraw;\
	client->lastDrawableID = drawID;\
	client->lastGC = pGC;\
	client->lastGCID = stuff->gc;\
    }\
    else\
    {\
        pGC = client->lastGC;\
        pDraw = client->lastDrawable;\
    }\
    if (pGC->serialNumber != pDraw->serialNumber)\
	ValidateGC(pDraw, pGC);

#endif /* OLDR4 */

#define WriteReplyToClient(pClient, size, pReply) \
   if ((pClient)->swapped) \
      (*ReplySwapVector[((xReq *)(pClient)->requestBuffer)->reqType]) \
           (pClient, (int)(size), pReply); \
      else (void) WriteToClient(pClient, (int)(size), (char *)(pReply));

#define WriteSwappedDataToClient(pClient, size, pbuf) \
   if ((pClient)->swapped) \
      (*(pClient)->pSwapReplyFunc)(pClient, (int)(size), pbuf); \
   else (void) WriteToClient (pClient, (int)(size), (char *)(pbuf));

typedef struct _TimeStamp *TimeStampPtr;
typedef struct _Client *ClientPtr;

typedef struct _WorkQueue	*WorkQueuePtr;

extern ClientPtr requestingClient;
extern ClientPtr *clients;
extern ClientPtr serverClient;
extern int currentMaxClients;
extern long *checkForInput[2];

extern int ProcAllowEvents();
extern int ProcBell();
extern int ProcChangeActivePointerGrab();
extern int ProcChangeKeyboardControl();
extern int ProcChangePointerControl();
extern int ProcGetKeyboardMapping();
extern int ProcGetPointerMapping();
extern int ProcGetInputFocus();
extern int ProcGetKeyboardControl();
extern int ProcGetMotionEvents();
extern int ProcGetPointerControl();
extern int ProcGrabButton();
extern int ProcGrabKey();
extern int ProcGrabKeyboard();
extern int ProcGrabPointer();
extern int ProcQueryKeymap();
extern int ProcQueryPointer();
extern int ProcSetInputFocus();
extern int ProcSetKeyboardMapping();
extern int ProcSetPointerMapping();
extern int ProcSendEvent();
extern int ProcUngrabButton();
extern int ProcUngrabKey();
extern int ProcUngrabKeyboard();
extern int ProcUngrabPointer();
extern int ProcWarpPointer();
extern int ProcRecolorCursor();

extern WindowPtr LookupWindow();
extern pointer LookupDrawable();

extern void NoopDDA();

#endif /* DIX_H */
