/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Footer.c	1.1"
#endif

/*  Example of the Footer widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/Footer.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
void
FooterCB OLARGLIST((w, client_data,  call_data))
	OLARG(Widget, w)
	OLARG(XtPointer, client_data)
	OLGRA(XtPointer, call_data)
{
	Widget popup, upper, lower, footer_panel;

	popup = XtVaCreatePopupShell("popupWindowShell",
		popupWindowShellWidgetClass, XtParent(w), 
		XtNresizeCorners, True,
		XtNtitle, "WidgetTree: Footer",
		(String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

	XtVaGetValues(popup, XtNfooterPanel, &footer_panel,
			XtNlowerControlArea, &lower,
			XtNupperControlArea, &upper,
			(String) 0);

	XtVaSetValues(upper, XtNwidth, 295, (String)0);

	XtVaCreateManagedWidget("footer", footerWidgetClass, footer_panel,
		XtNleftFoot, "Left footer message",
		XtNleftWeight, 1,
		XtNrightFoot, "Right footer message",
		XtNrightWeight, 1,
		(String) 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, lower,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
			OL_DISK_SOURCE, "Footer.c");

	XtPopup(popup, XtGrabNone);
}  /* end of FooterCB() */
