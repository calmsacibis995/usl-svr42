/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)Dt:dtmReq.c	1.9"
#endif

#include <stdio.h>
#include <X11/Intrinsic.h>
#include "X11/Xatom.h"
#include "DesktopI.h"

#define OFFSET(P)	XtOffset(DtOpenFolderRequest *, P)
static DtStrToStructMapping const open_folder[] = {
	REQUEST_HDR_MAPPING
	{ "TITLE",	OFFSET(title),		DT_MTYPE_STRING },
	{ "PATH",	OFFSET(path),		DT_MTYPE_STRING },
	{ "OPTIONS",	OFFSET(options),	DT_MTYPE_ULONG  },
	{ "PATTERN",	OFFSET(pattern),	DT_MTYPE_STRING },
	{ "FILECLASS",	OFFSET(class_name),	DT_MTYPE_STRING },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtSyncFolderRequest *, P)
static DtStrToStructMapping const sync_folder[] = {
	REQUEST_HDR_MAPPING
	{ "PATH",	OFFSET(path),		DT_MTYPE_STRING },
	{ "OPTIONS",	OFFSET(options),	DT_MTYPE_ULONG  },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtDisplayPropSheetRequest *, P)
static DtStrToStructMapping const display_prop_sheet[] = {
	REQUEST_HDR_MAPPING
	{ "PROP_NAME",	OFFSET(prop_name),	DT_MTYPE_STRING },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtDisplayBinderRequest *, P)
static DtStrToStructMapping const display_binder[] = {
	REQUEST_HDR_MAPPING
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtOpenFMapRequest *, P)
static DtStrToStructMapping const open_fmap[] = {
	REQUEST_HDR_MAPPING
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtShutdownRequest *, P)
static DtStrToStructMapping const dt_shutdown[] = {
	REQUEST_HDR_MAPPING
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtQueryFclassRequest *, P)
static DtStrToStructMapping const query_class[] = {
	REQUEST_HDR_MAPPING
	{ "FILECLASS",	OFFSET(class_name),	DT_MTYPE_STRING },
	{ "OPTIONS",	OFFSET(options),	DT_MTYPE_ULONG  },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtCreateFclassRequest *, P)
static DtStrToStructMapping const create_class[] = {
	REQUEST_HDR_MAPPING
	{ "FILENAME",	OFFSET(file_name),	DT_MTYPE_STRING },
	{ "OPTIONS",	OFFSET(options),	DT_MTYPE_ULONG },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtDeleteFclassRequest *, P)
static DtStrToStructMapping const delete_class[] = {
	REQUEST_HDR_MAPPING
	{ "FILENAME",	OFFSET(file_name),	DT_MTYPE_STRING },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtGetDesktopPropertyRequest *, P)
static DtStrToStructMapping const get_property[] = {
	REQUEST_HDR_MAPPING
	{ "NAME",	OFFSET(name),		DT_MTYPE_STRING },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtSetDesktopPropertyRequest *, P)
static DtStrToStructMapping const set_property[] = {
	REQUEST_HDR_MAPPING
	{ "NAME",	OFFSET(name),		DT_MTYPE_STRING },
	{ "VALUE",	OFFSET(value),		DT_MTYPE_STRING },
	{ "ATTRS",	OFFSET(attrs),		DT_MTYPE_ULONG  },
};
#undef OFFSET

DtMsgInfo const Dt__dtm_msgtypes[] = {
{ "OPEN_FOLDER",  DT_OPEN_FOLDER,      open_folder,  XtNumber(open_folder) },
{ "SYNC_FOLDER",  DT_SYNC_FOLDER,      sync_folder,  XtNumber(sync_folder) },
{ "CREATE_CLASS", DT_CREATE_FILE_CLASS,create_class, XtNumber(create_class) },
{ "DELETE_CLASS", DT_DELETE_FILE_CLASS,delete_class, XtNumber(delete_class) },
{ "QUERY_CLASS",  DT_QUERY_FILE_CLASS, query_class,  XtNumber(query_class) },
{ "GET_PROPERTY", DT_GET_DESKTOP_PROPERTY,get_property,XtNumber(get_property) },
{ "SET_PROPERTY", DT_SET_DESKTOP_PROPERTY,set_property,XtNumber(set_property) },
{ "DISPLAY_PROP_SHEET", DT_DISPLAY_PROP_SHEET,display_prop_sheet,XtNumber(display_prop_sheet) },
{ "DISPLAY_BINDER", DT_DISPLAY_BINDER,display_binder,XtNumber(display_binder) },
{ "OPEN_FMAP", DT_OPEN_FMAP,open_fmap,XtNumber(open_fmap) },
{ "SHUTDOWN", DT_SHUTDOWN,dt_shutdown,XtNumber(dt_shutdown) },
};

/* reply structures */
#define OFFSET(P)	XtOffset(DtOpenFolderReply *, P)
static DtStrToStructMapping const ropen_folder[] = {
	REPLY_HDR_MAPPING
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtQueryFclassReply *, P)
static DtStrToStructMapping const rquery_class[] = {
	REPLY_HDR_MAPPING
	{ "FILECLASS",	OFFSET(class_name),	DT_MTYPE_STRING },
	{ "PROPERTIES",	OFFSET(plist),		DT_MTYPE_PLIST  },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtCreateFclassReply *, P)
static DtStrToStructMapping const rcreate_class[] = {
	REPLY_HDR_MAPPING
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtDeleteFclassReply *, P)
static DtStrToStructMapping const rdelete_class[] = {
	REPLY_HDR_MAPPING
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtGetDesktopPropertyReply *, P)
static DtStrToStructMapping const rget_property[] = {
	REPLY_HDR_MAPPING
	{ "VALUE",	OFFSET(value),		DT_MTYPE_STRING },
	{ "ATTRS",	OFFSET(attrs),		DT_MTYPE_ULONG  },
};
#undef OFFSET

DtMsgInfo const Dt__dtm_replytypes[] = {
{ "OPEN_FOLDER", DT_OPEN_FOLDER, ropen_folder,  XtNumber(ropen_folder) },
{ "QUERY_CLASS", DT_QUERY_FILE_CLASS, rquery_class,  XtNumber(rquery_class) },
{ "CREATE_CLASS",DT_CREATE_FILE_CLASS,rcreate_class, XtNumber(rcreate_class) },
{ "DELETE_CLASS",DT_DELETE_FILE_CLASS,rdelete_class, XtNumber(rdelete_class) },
{ "GET_PROPERTY",DT_GET_DESKTOP_PROPERTY,rget_property,XtNumber(rget_property)},
};

