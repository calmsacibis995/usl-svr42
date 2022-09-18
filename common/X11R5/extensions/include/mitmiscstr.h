/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:include/mitmiscstr.h	1.1"
/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology


********************************************************/

/* RANDOM CRUFT! THIS HAS NO OFFICIAL X CONSORTIUM BLESSING */

/* $XConsortium: mitmiscstr.h,v 1.3 90/05/15 18:50:18 keith Exp $ */

#include "MITMisc.h"

#define MITMISCNAME "MIT-SUNDRY-NONSTANDARD"

typedef struct _SetBugMode {
    CARD8	reqType;	/* always MITReqCode */
    CARD8	mitReqType;	/* always X_MITSetBugMode */
    CARD16	length B16;
    BOOL	onOff;
    BYTE	pad0;
    CARD16	pad1;
} xMITSetBugModeReq;
#define sz_xMITSetBugModeReq	8

typedef struct _GetBugMode {
    CARD8	reqType;	/* always MITReqCode */
    CARD8	mitReqType;	/* always X_MITGetBugMode */
    CARD16	length B16;
} xMITGetBugModeReq;
#define sz_xMITGetBugModeReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	onOff;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	pad0 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xMITGetBugModeReply;
#define sz_xMITGetBugModeReply	32

