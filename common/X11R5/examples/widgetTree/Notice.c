/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Notice.c	1.1"
#endif

/*  Example of the NoticeShell widget  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Notice.h>
#include <Xol/FButtons.h>
#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
void
NoticeCB OLARGLIST((w, client_data,  call_data))
	OLARG(Widget, w)
	OLARG(XtPointer, client_data)
	OLGRA(XtPointer, call_data)
{
	Widget notice, controlArea;

	notice = XtVaCreatePopupShell("noticeShell",
		noticeShellWidgetClass, XtParent(w), 
		XtNstring, "Information:  NoticeShell displays messages",
		XtNtitle, "Information Dialog",
		XtNnoticeType, OL_INFORMATION,
		XtNemanateWidget, w,
		(String)0);
	XtAddCallback(notice, XtNpopdownCallback, DestroyCB, w);

	XtVaGetValues(notice, XtNcontrolArea, &controlArea, 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, controlArea,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, 2,
		XtNclientData, notice,
                (String) 0);

	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) notice, NULL,
			OL_DISK_SOURCE, "Notice.c");

	XtPopup(notice, XtGrabExclusive);
}  /* end of NoticeCB() */
