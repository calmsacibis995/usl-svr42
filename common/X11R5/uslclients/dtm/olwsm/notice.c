/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/notice.c	1.16"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/Notice.h>
#include <Xol/FButtons.h>

#include <misc.h>
#include <list.h>
#include <notice.h>
#include <xtarg.h>

char * NoticeMenuFields[] ={
        XtNdefault,
        XtNselectProc,
        XtNlabel,
        XtNmnemonic
};


/**
 ** CreateNoticeBox()
 **/

void
#if	OlNeedFunctionPrototypes
CreateNoticeBox (
	Widget			parent,
	Notice *		notice
)
#else
CreateNoticeBox (parent, notice)
	Widget			parent;
	Notice *		notice;
#endif
{
	Screen *		screen	= XtScreenOfObject(parent);

	Widget			textarea;
	Widget			controlarea;


	notice->w = XtVaCreatePopupShell(
		notice->name,
		noticeShellWidgetClass,
		parent,
		(String)0
	);
	XtVaGetValues (
		notice->w,
		XtNtextArea,    (XtArgVal)&textarea,
		XtNcontrolArea, (XtArgVal)&controlarea,
		(String)0
	);

	/*
	 * Arrrggghhh...the NoticeShell forces these two resources
	 * if not given programmatically.
	 */
#define _8HorzPoints	OlScreenPointToPixel(OL_HORIZONTAL,8,screen)
#define _20HorzPoints	OlScreenPointToPixel(OL_HORIZONTAL,20,screen)
#define _8VertPoints	OlScreenPointToPixel(OL_VERTICAL,8,screen)
#define _30VertPoints	OlScreenPointToPixel(OL_VERTICAL,30,screen)
	XtVaSetValues (
		controlarea,
		XtNvPad,	(XtArgVal)_8VertPoints,
		XtNhPad,	(XtArgVal)_20HorzPoints,
		XtNhSpace,	(XtArgVal)_8HorzPoints,
		(String)0
	);

	XtVaSetValues (
		textarea,
		XtNstring,	(XtArgVal)notice->string,
		XtNvSpace,	(XtArgVal)_30VertPoints,
		XtNhSpace,	(XtArgVal)_20HorzPoints,
		(String)0
	);
	XtVaSetValues (
		XtParent ( textarea ),		/* "pane" widget */
		XtNhPad,	(XtArgVal)0,
		XtNvPad,	(XtArgVal)0,
		XtNvSpace,	(XtArgVal)0,
		(String)0
	);

	XtVaCreateManagedWidget(
		notice->name,
		flatButtonsWidgetClass,
		controlarea,
		XtNitemFields,		(XtArgVal)NoticeMenuFields,
		XtNnumItemFields,	(XtArgVal)NUM_FIELDS,
		XtNitems,		(XtArgVal)notice->items,
		XtNnumItems,		(XtArgVal)notice->numitems,
		(String)0
	);
} /* CreateNoticeBox */
