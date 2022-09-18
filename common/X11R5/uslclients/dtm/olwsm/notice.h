/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/notice.h	1.10"
#endif

#ifndef _NOTICE_H
#define _NOTICE_H

#define NUM_FIELDS 4

typedef struct NoticeItem {
	XtArgVal	flag;
	XtArgVal	function;
	XtArgVal	string;
	XtArgVal	mnemonic;
} NoticeItem;

typedef struct Notice {
	String			name;
	String			string;
	NoticeItem *		items;	/* of NoticeItem struct's */
	int			numitems;
	Widget			w;	/* holds the notice widget */
}			Notice;

extern void		CreateNoticeBox OL_ARGS((
	Widget			parent,
	Notice *		notice
));

#endif
