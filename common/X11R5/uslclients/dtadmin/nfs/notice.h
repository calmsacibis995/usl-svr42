/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/notice.h	1.3"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	notice.h      notice box header
 */

extern void DeleteCB();

typedef struct _noticeData
{
    char          *text;
    char          *label;
    char          *mnemonic;
    XtPointer      callBack;	/* really XtCallBackProc */
    XtPointer	   client_data;
	
} noticeData;

typedef enum _noticeIndex
{
    NoticeDoIt, NoticeCancel, NoticeHelp
} noticeIndex;

extern MenuItems  NoticeMenuItems[];
extern MenuGizmo NoticeMenu;
extern ModalGizmo noticeG;
