/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/dixstruct.h	1.4"

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
******************************************************************/
/* $XConsortium: dixstruct.h,v 1.15 91/02/14 19:35:59 keith Exp $ */

#ifndef DIXSTRUCT_H
#define DIXSTRUCT_H

#include "dix.h"
#include "resource.h"
#include "cursor.h"
#include "gc.h"
#include "pixmap.h"

/*
 * 	direct-mapped hash table, used by resource manager to store
 *      translation from client ids to server addresses.
 */

typedef struct _TimeStamp {
    unsigned long months;	/* really ~49.7 days */
    unsigned long milliseconds;
}           TimeStamp;

#ifdef DEBUG
#define MAX_REQUEST_LOG 100
#endif

typedef struct _Client {
    int         index;
    Mask        clientAsMask;
    pointer     requestBuffer;
    pointer     osPrivate;	/* for OS layer, including scheduler */
    Bool        swapped;
    void        (*pSwapReplyFunc) ();
    XID         errorValue;
    int         sequence;
    int         closeDownMode;
    int         clientGone;
    int         noClientException;	/* this client died or needs to be
					 * killed */
    DrawablePtr lastDrawable;
    Drawable    lastDrawableID;
    GCPtr       lastGC;
    GContext    lastGCID;
    pointer    *saveSet;
    int         numSaved;
    pointer     screenPrivate[MAXSCREENS];
    int         (**requestVector) ();

#ifdef DEBUG
    unsigned char requestLog[MAX_REQUEST_LOG];
    int         requestLogIndex;
#endif
}           ClientRec;

typedef struct _WorkQueue {
    struct _WorkQueue *next;
    Bool        (*function) ();
    ClientPtr   client;
    pointer     closure;
}           WorkQueueRec;

extern TimeStamp currentTime;
extern void CloseDownClient();

extern TimeStamp ClientTimeToServerTime();
extern void UpdateCurrentTime();
extern void UpdateCurrentTimeIf();

#endif				/* DIXSTRUCT_H */
