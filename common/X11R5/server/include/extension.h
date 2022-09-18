/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/extension.h	1.3"

/* $XConsortium: extension.h,v 1.6 89/07/16 14:37:47 rws Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
#ifndef EXTENSION_H
#define EXTENSION_H 

#define GetGCAndDrawableAndValidate(gcID, pGC, drawID, pDraw, client)\
    if ((client->lastDrawableID != drawID) || (client->lastGCID != gcID))\
    {\
        if (client->lastDrawableID != drawID)\
    	    pDraw = (DrawablePtr)LookupIDByClass(drawID, RC_DRAWABLE);\
        else\
	    pDraw = client->lastDrawable;\
        if (client->lastGCID != gcID)\
	    pGC = (GC *)LookupIDByType(gcID, RT_GC);\
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
            client->lastGCID = gcID;\
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
        client->errorValue = gcID;\
        return (BadGC);\
    }\
    if (pGC->serialNumber != pDraw->serialNumber)\
	ValidateGC(pDraw, pGC);
#endif /* EXTENSION_H */
