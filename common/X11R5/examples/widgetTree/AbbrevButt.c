/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:AbbrevButt.c	1.1"
#endif

/*  Example of the AbbreviatedButton widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/PopupMenu.h>
#include <Xol/OblongButt.h>
#include <Xol/AbbrevButt.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

static item menu_items[] = {
	{ "Restart", NULL},
	{ "Cancel", NULL},
	{ "Abort", NULL}
};

/*ARGSUSED*/
void
AbbreviatedButtonCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, window, menu;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetClass: AbbreviatedMenuButton",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

	window = XtVaCreatePopupShell("popup", popupWindowShellWidgetClass, popup,
		XtNtitle, "AbbreviatedButton Window",
		(String) 0);

	caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper,
		XtNlabel, "AbbreviatedButton (Window):",
		XtNfont, (XFontStruct *) _OlGetDefaultFont(upper, OlDefaultBoldFont),
		(String) 0);

        XtVaCreateManagedWidget("abbrevbuttonwindow",
		abbreviatedButtonWidgetClass, caption,
		XtNbuttonType, OL_WINDOW_BTN,
		XtNpopupWidget, window,
		(String)0);

	menu = XtCreatePopupShell("menu", popupMenuShellWidgetClass, popup,
		NULL, 0);

	XtVaCreateManagedWidget("buttons", flatButtonsWidgetClass, menu,
		XtNitems, menu_items,
		XtNnumItems, XtNumber(menu_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		(String) NULL);

	caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper,
		XtNlabel, "AbbreviatedButton (Menu):",
		XtNfont, _OlGetDefaultFont(upper, OlDefaultBoldFont),
		(String) 0);

        XtVaCreateManagedWidget("abbrevbuttonmenu",
		abbreviatedButtonWidgetClass, caption,
		XtNbuttonType, OL_MENU_BTN,
		XtNpopupWidget, menu,
		(String)0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, lower,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "AbbrevButt.c");

        XtPopup(popup, XtGrabNone);
}  /* end of AbbreviatedButtonCB() */
