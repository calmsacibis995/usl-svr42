/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Modal.c	1.1"
#endif

/*  Example of the ModalShell widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Modal.h>

#include <Xol/StaticText.h>
#include <Xol/FButtons.h>
#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
static void
PopdownCB OLARGLIST((w, client_data,  call_data))
	OLARG(Widget, w)
	OLARG(XtPointer, client_data)
	OLGRA(XtPointer, call_data)
{
	XtVaSetValues((Widget) client_data, XtNbusy, False, 0);

	XtPopdown((Widget) XtParent(w));
}  /* end of PopdownCB() */

/*ARGSUSED*/
void
ModalCB OLARGLIST((w, client_data,  call_data))
	OLARG(Widget, w)
	OLARG(XtPointer, client_data)
	OLGRA(XtPointer, call_data)
{
	Widget modal;

	modal = XtVaCreatePopupShell("modalShell",
		modalShellWidgetClass, XtParent(w), 
		(String)0);
	XtAddCallback(modal, XtNpopdownCallback, DestroyCB, w);

	XtVaCreateManagedWidget("staticText", staticTextWidgetClass, modal,
		XtNstring, "Error:  ModalShell displays modal messages",
		XtNemanateWidget, w,
		XtNgravity, OL_CENTER,
		(String) 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, modal,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, modal,
                (String) 0);

	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) modal, NULL,
			OL_DISK_SOURCE, "Modal.c");

	XtVaSetValues(w, XtNbusy, True, 0);

	XtPopup(modal, XtGrabExclusive);
}  /* end of ModalCB() */
