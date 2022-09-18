/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:FList.c	1.2"
#endif

/*  Example of the Flat List widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/ScrolledWi.h>
#include <Xol/FList.h>

#include "WidgetTree.h"

static item lower_items[] = {
	{ "OK", (XtPointer) PopdownCB },
};

static item color_items[] = {
	{ "Red", NULL},
	{ "Green", NULL},
	{ "Blue", NULL},
	{ "Orange", NULL},
	{ "Purple", NULL},
	{ "Pink", NULL},
	{ "Black", NULL},
	{ "White", NULL},
};

/*ARGSUSED*/
void
FListCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, sw;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: FlatList",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

        caption  = XtVaCreateManagedWidget("List", captionWidgetClass,
		upper,
                XtNlabel, "Flat List:",
		(String) 0);

	sw = XtVaCreateManagedWidget("sw", scrolledWindowWidgetClass,
		caption, 
		XtNforceVerticalSB, True,
		(String) 0);

        XtVaCreateManagedWidget("flist",
		flatListWidgetClass, sw,
		XtNexclusives, True,
		XtNviewHeight, 4,
		XtNitems, color_items,
		XtNnumItems, XtNumber(color_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, lower,
		XtNitems, lower_items,
		XtNnumItems, XtNumber(lower_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "FList.c");

        XtPopup(popup, XtGrabNone);
}  /* end of FListCB() */
