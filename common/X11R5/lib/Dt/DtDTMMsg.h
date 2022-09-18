/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)Dt:DtDTMMsg.h	1.8"
#endif

#ifndef __DtDTMMsg_h__
#define __DtDTMMsg_h__

/*
 **************************************************************************
 *
 * Description:
 *              This file contains all the things related to DTM request
 *       and reply messages.
 *
 **************************************************************************
 */

/* message function */
#define DTM_MSG		1
#define DTM_FUNC	(DTM_MSG << MSGFUNC_SHIFT)

/* functional specific request types */
#define DT_OPEN_FOLDER		(DTM_FUNC | 1)
#define DT_SYNC_FOLDER		(DTM_FUNC | 2)
#define DT_CREATE_FILE_CLASS	(DTM_FUNC | 3)
#define DT_DELETE_FILE_CLASS	(DTM_FUNC | 4)
#define DT_QUERY_FILE_CLASS	(DTM_FUNC | 5)
#define DT_GET_DESKTOP_PROPERTY	(DTM_FUNC | 6)
#define DT_SET_DESKTOP_PROPERTY	(DTM_FUNC | 7)
#define DT_DISPLAY_PROP_SHEET  	(DTM_FUNC | 8)
#define DT_DISPLAY_BINDER      	(DTM_FUNC | 9)
#define DT_OPEN_FMAP		(DTM_FUNC | 10)
#define DT_SHUTDOWN		(DTM_FUNC | 11)

#define DT_DTM_NUM_REQUESTS	11
#define DT_DTM_NUM_REPLIES	5

typedef struct {
	REQUEST_HDR
	char		*title;		/* title of folder window */
	char		*path;		/* full path of directory */
	DtAttrs		options;	/* options */
	char		*pattern;	/* filename pattern */
	char		*class_name;	/* file class name */
} DtOpenFolderRequest;

typedef struct {
	REQUEST_HDR
	char		*path;		/* full path of directory */
	DtAttrs		options;	/* options */
} DtSyncFolderRequest;

typedef struct {
	REQUEST_HDR
	char		*prop_name;	/* name of property sheet */
} DtDisplayPropSheetRequest;

typedef struct {
	REQUEST_HDR
} DtDisplayBinderRequest;

typedef struct {
	REQUEST_HDR
} DtOpenFMapRequest;

typedef struct {
	REQUEST_HDR
} DtShutdownRequest;

typedef struct {
	REQUEST_HDR
	char		*class_name;	/* file class name */
	DtAttrs		options;	/* options */
} DtQueryFclassRequest;

typedef struct {
	REQUEST_HDR
	char		*file_name;	/* class database file name */
} DtDeleteFclassRequest;

typedef struct {
	REQUEST_HDR
	char		*file_name;	/* class database file name */
	DtAttrs		options;	/* options */
} DtCreateFclassRequest;

typedef struct {
	REQUEST_HDR
	char		*name;		/* property name */
} DtGetDesktopPropertyRequest;

typedef struct {
	REQUEST_HDR
	char		*name;		/* property name */
	char		*value;		/* property value */
	DtAttrs		attrs;		/* attributes */
} DtSetDesktopPropertyRequest;

typedef struct {
	REPLY_HDR
} DtOpenFolderReply;

typedef struct {
	REPLY_HDR
	char		*class_name;	/* file class name */
	DtPropList	plist;		/* property list */
} DtQueryFclassReply;

typedef struct {
	REPLY_HDR
} DtCreateFclassReply;

typedef struct {
	REPLY_HDR
} DtDeleteFclassReply;

typedef struct {
	REPLY_HDR
	char		*value;		/* property value */
	DtAttrs		attrs;		/* attributes */
} DtGetDesktopPropertyReply;

extern DtMsgInfo const Dt__dtm_msgtypes[DT_DTM_NUM_REQUESTS];
extern DtMsgInfo const Dt__dtm_replytypes[DT_DTM_NUM_REPLIES];
#endif /* __DtDTMMsg_h__ */
