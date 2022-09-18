/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)Dt:wbReq.c	1.8"
#endif

#include <stdio.h>
#include <X11/Intrinsic.h>
#include "X11/Xatom.h"
#include "DesktopI.h"

/* request structures */
#define OFFSET(P)	XtOffset(DtMoveToWBRequest *, P)
static DtStrToStructMapping const move_to_wb[] = {
	REQUEST_HDR_MAPPING
	{ "PATH",	OFFSET(pathname),	DT_MTYPE_STRING },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtMoveFromWBRequest *, P)
static DtStrToStructMapping const move_from_wb[] = {
	REQUEST_HDR_MAPPING
	{ "PATH",	OFFSET(pathname),	DT_MTYPE_STRING },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtDisplayWBRequest *, P)
static DtStrToStructMapping const display_wb[] = {
	REQUEST_HDR_MAPPING
};
#undef OFFSET

DtMsgInfo const Dt__wb_msgtypes[] = {
 { "MOVE_TO_WB",   DT_MOVE_TO_WB,   move_to_wb,   XtNumber(move_to_wb)   },
 { "MOVE_FROM_WB", DT_MOVE_FROM_WB, move_from_wb, XtNumber(move_from_wb) },
 { "DISPLAY_WB",   DT_DISPLAY_WB,   display_wb,   XtNumber(display_wb)   },
};

/* reply structures */
#define OFFSET(P)	XtOffset(DtMoveToWBReply *, P)
static DtStrToStructMapping const move_to_wb_reply[] = {
	REPLY_HDR_MAPPING
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtMoveFromWBReply *, P)
static DtStrToStructMapping const move_from_wb_reply[] = {
	REPLY_HDR_MAPPING
};
#undef OFFSET

DtMsgInfo const Dt__wb_replytypes[] = {
{ "MOVE_TO_WB",   DT_MOVE_TO_WB,   move_to_wb_reply,
	XtNumber(move_to_wb_reply) },
{ "MOVE_FROM_WB", DT_MOVE_FROM_WB, move_from_wb_reply,
	XtNumber(move_from_wb_reply) },
};
