/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)Dt:DtTBMsg.h	1.3"
#endif

#ifndef __DtTMMsg_h__
#define __DtTMMsg_h__

/*
 **************************************************************************
 *
 * Description:
 *              This file contains all the things related to TM request
 *       and reply messages.
 *
 **************************************************************************
 */

/* message function */
#define TM_FUNC	(TOOLBOX_MSG << MSGFUNC_SHIFT)

/* functional specific request types */
#define DT_OPEN_TOOLBOX			(TM_FUNC | 1)
#define DT_CREATE_TOOLBOX_OBJECT	(TM_FUNC | 2)
#define DT_DELETE_TOOLBOX_OBJECT	(TM_FUNC | 3)
#define DT_QUERY_TOOLBOX_OBJECT		(TM_FUNC | 4)

#define DT_TB_NUM_REQUESTS	4
#define DT_TB_NUM_REPLIES	3

typedef struct {
	REQUEST_HDR
	char		*path;		/* path & name of object */
} DtOpenToolboxRequest;

typedef struct {
	REQUEST_HDR
	char		*path;		/* path & name of object */
	char		*objtype;	/* object type */
	DtPropList	plist;		/* property list */
} DtCreateToolboxObjectRequest;

typedef struct {
	REQUEST_HDR
	char		*path;		/* path & name of object */
	DtAttrs		options;	/* options */
} DtQueryToolboxObjectRequest;

typedef struct {
	REQUEST_HDR
	char		*path;		/* path & name of object */
} DtDeleteToolboxObjectRequest;

typedef struct {
	REPLY_HDR
} DtCreateToolboxObjectReply;

typedef struct {
	REPLY_HDR
	char		*objtype;	/* object type */
	DtPropList	plist;		/* property list */
} DtQueryToolboxObjectReply;

typedef struct {
	REPLY_HDR
} DtDeleteToolboxObjectReply;

extern DtMsgInfo const Dt__tb_msgtypes[DT_TB_NUM_REQUESTS];
extern DtMsgInfo const Dt__tb_replytypes[DT_TB_NUM_REPLIES];

#endif /* __DtTMMsg_h__ */
