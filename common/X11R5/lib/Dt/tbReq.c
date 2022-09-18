/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)Dt:tbReq.c	1.3"
#endif

#include <stdio.h>
#include <X11/Intrinsic.h>
#include "X11/Xatom.h"
#include "DesktopI.h"

#define OFFSET(P)	XtOffset(DtOpenToolboxRequest *, P)
static DtStrToStructMapping const open_tb[] = {
	REQUEST_HDR_MAPPING
	{ "PATH",	OFFSET(path),		DT_MTYPE_STRING },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtCreateToolboxObjectRequest *, P)
static DtStrToStructMapping const create_tb_obj[] = {
	REQUEST_HDR_MAPPING
	{ "PATH",	OFFSET(path),		DT_MTYPE_STRING },
	{ "OBJTYPE",	OFFSET(objtype),	DT_MTYPE_STRING },
	{ "PROPERTIES",	OFFSET(plist),		DT_MTYPE_PLIST },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtQueryToolboxObjectRequest *, P)
static DtStrToStructMapping const query_tb_obj[] = {
	REQUEST_HDR_MAPPING
	{ "PATH",	OFFSET(path),		DT_MTYPE_STRING },
	{ "OPTIONS",	OFFSET(options),	DT_MTYPE_ULONG  },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtDeleteToolboxObjectRequest *, P)
static DtStrToStructMapping const delete_tb_obj[] = {
	REQUEST_HDR_MAPPING
	{ "PATH",	OFFSET(path),		DT_MTYPE_STRING },
};
#undef OFFSET

DtMsgInfo const Dt__tb_msgtypes[] = {
{ "OPEN_TOOLBOX", DT_OPEN_TOOLBOX, open_tb, XtNumber(open_tb) },
{ "CREATE_TOOLBOX_OBJECT", DT_CREATE_TOOLBOX_OBJECT, create_tb_obj, XtNumber(create_tb_obj) },
{ "DELETE_TOOLBOX_OBJECT", DT_DELETE_TOOLBOX_OBJECT, delete_tb_obj, XtNumber(delete_tb_obj) },
{ "QUERY_TOOLBOX_OBJECT",  DT_QUERY_TOOLBOX_OBJECT,  query_tb_obj,  XtNumber(query_tb_obj) },
};

/* reply structures */
#define OFFSET(P)	XtOffset(DtCreateToolboxObjectReply *, P)
static DtStrToStructMapping const rcreate_tb_obj[] = {
	REPLY_HDR_MAPPING
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtQueryToolboxObjectReply *, P)
static DtStrToStructMapping const rquery_tb_obj[] = {
	REPLY_HDR_MAPPING
	{ "OBJTYPE",	OFFSET(objtype),	DT_MTYPE_STRING },
	{ "PROPERTIES",	OFFSET(plist),		DT_MTYPE_PLIST },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtDeleteToolboxObjectReply *, P)
static DtStrToStructMapping const rdelete_tb_obj[] = {
	REPLY_HDR_MAPPING
};
#undef OFFSET

DtMsgInfo const Dt__tb_replytypes[] = {
{ "CREATE_TOOLBOX_OBJECT", DT_CREATE_TOOLBOX_OBJECT, rcreate_tb_obj, XtNumber(rcreate_tb_obj) },
{ "DELETE_TOOLBOX_OBJECT", DT_DELETE_TOOLBOX_OBJECT, rdelete_tb_obj, XtNumber(rdelete_tb_obj) },
{ "QUERY_TOOLBOX_OBJECT",  DT_QUERY_TOOLBOX_OBJECT,  rquery_tb_obj,  XtNumber(rquery_tb_obj) },
};

