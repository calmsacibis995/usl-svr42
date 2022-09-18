/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Caption.c	1.1"
#endif

/*  Example of the Caption widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/AbbrevMenu.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

static item menu_items[] = {
	{ "Open...", NULL},
	{ "Save...", NULL},
	{ "Exit", NULL},
};

/*ARGSUSED*/
void
CaptionCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, abbrevmenu, pane;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "Caption",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

	caption = XtVaCreateManagedWidget("leftcaption", captionWidgetClass, upper,
		XtNlabel, "Position Left:",
		XtNfont, (XFontStruct *)_OlGetDefaultFont(upper, OlDefaultBoldFont),
		XtNposition, OL_LEFT,
		(String) 0);

	caption = XtVaCreateManagedWidget("rightCaption", captionWidgetClass, caption,
		XtNlabel, ":Position Right",
		XtNfont, _OlGetDefaultFont(upper, OlDefaultBoldFont),
		XtNposition, OL_RIGHT,
		XtNborderWidth, 1,
		(String) 0);

	caption = XtVaCreateManagedWidget("topCaption", captionWidgetClass, caption,
		XtNlabel, "Position Top:",
		XtNfont, _OlGetDefaultFont(upper, OlDefaultBoldFont),
		XtNposition, OL_TOP,
		XtNborderWidth, 1,
		(String) 0);

	caption = XtVaCreateManagedWidget("topCaption", captionWidgetClass, caption,
		XtNlabel, "Position Bottom:",
		XtNfont, _OlGetDefaultFont(upper, OlDefaultBoldFont),
		XtNposition, OL_BOTTOM,
		XtNborderWidth, 1,
		(String) 0);

        abbrevmenu = XtVaCreateManagedWidget("abbrevmenu",
		abbrevMenuButtonWidgetClass, caption,
		(String)0);

        XtVaGetValues(abbrevmenu, XtNmenuPane, &pane, (String) 0);

        XtVaCreateManagedWidget("menuitems",
		flatButtonsWidgetClass, pane,
		XtNitems, menu_items,
		XtNnumItems, XtNumber(menu_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, NULL,
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
                        OL_DISK_SOURCE, "Caption.c");

        XtPopup(popup, XtGrabNone);
}  /* end of CaptionCB() */
