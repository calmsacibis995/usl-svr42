/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)scrollinglist:ListPane.h	1.7"
#endif
/*
 ListPane.h (C hdr file)
	Acc: 596865503 Tue Nov 29 22:58:23 1988
	Mod: 596865503 Tue Nov 29 22:58:23 1988
	Sta: 596865503 Tue Nov 29 22:58:23 1988
	Owner: 4777
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
#ifndef _ListPane_h
#define _ListPane_h

/*
 * OPEN LOOK(TM) ListPane Widget
 */

#include <Xol/Primitive.h>		/* include superclasses' header */

/*
 * New Resources:
 *
 * Name			Type		Default		Meaning
 * ----			----		-------		-------
 * XtNdelete		Token		n/a		delete item from list
 * XtNeditOn		Token		n/a		edit item in list
 * XtNeditOff		Boolean		FALSE		done editing
 * XtNfont		XFontStuct	---		font
 * XtNfontColor		Pixel		Black		font color
 * XtNinsert		Token		n/a		insert item into list
 * XtNmove		Token		n/a		move to node in tree
 * XtNrecomputeWidth	Boolean		TRUE		resize or live w/ size
 * XtNselectable	Boolean		True		SINGLE / MULTI_LEVEL
 * XtNtouch		Token		n/a		list item has changed
 * XtNupdateView	Boolean		TRUE		lock/unlock view
 * XtNview		Token		n/a		view list item
 * XtNviewHeight	Cardinal	---		# items desired in view
 */

/* Class record pointer */
extern WidgetClass listPaneWidgetClass;

/* C Widget type definition */
typedef struct _ListPaneClassRec	*ListPaneWidgetClass;
typedef struct _ListPaneRec		*ListPaneWidget;

#endif /* _ListPane_h */
